#!/usr/bin/env bash
# Universal Spectrum Toolkit - basit menu
# ROOT bilmeyen kullanicilar icin kolay baslatici.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

RED='\033[0;31m'
GREEN='\033[0;32m'
CYAN='\033[0;36m'
YELLOW='\033[1;33m'
BOLD='\033[1m'
NC='\033[0m'

print_header() {
  clear 2>/dev/null || true
  echo -e "${CYAN}${BOLD}"
  echo "============================================================"
  echo "   Universal Spectrum Toolkit"
  echo "   SPE / CHN  -->  ROOT donusturucu"
  echo "============================================================"
  echo -e "${NC}"
}

print_menu() {
  echo -e "${BOLD}Ne yapmak istiyorsunuz?${NC}"
  echo
  echo "  1) Tek dosya donustur  (.spe veya .chn)"
  echo "  2) Klasordeki tum .spe / .chn dosyalarini donustur"
  echo "  3) Ornek dosyalarla dene (samples/)"
  echo "  4) Yardim / ne yapar?"
  echo "  5) Cikis"
  echo
}

require_root() {
  if ! command -v root >/dev/null 2>&1; then
    echo -e "${RED}HATA:${NC} 'root' komutu bulunamadi."
    echo "Once ROOT kurulu bir terminale gecin (WSL/Linux)."
    echo "Ornek: source ~/root/bin/thisroot.sh"
    exit 1
  fi
}

ask_yes_no() {
  local prompt="$1"
  local default="${2:-e}"
  local answer
  if [[ "$default" == "e" ]]; then
    read -r -p "$prompt [E/h]: " answer || true
    answer="${answer:-E}"
  else
    read -r -p "$prompt [e/H]: " answer || true
    answer="${answer:-H}"
  fi
  [[ "$answer" =~ ^[EeYy] ]]
}

strip_quotes() {
  local s="$1"
  s="${s%\"}"
  s="${s#\"}"
  s="${s%\'}"
  s="${s#\'}"
  printf '%s' "$s"
}

run_root_single() {
  local input_path="$1"
  local draw="$2"
  local force="$3"
  local logy="${4:-1}"

  local draw_cpp="true"
  local force_cpp="false"
  local logy_cpp="true"
  [[ "$draw" == "0" ]] && draw_cpp="false"
  [[ "$force" == "1" ]] && force_cpp="true"
  [[ "$logy" == "0" ]] && logy_cpp="false"

  # Dosya yolundaki ozel karakterler icin guvenli kacis
  local escaped
  escaped=$(printf '%s' "$input_path" | sed 's/\\/\\\\/g; s/"/\\"/g')

  echo
  echo -e "${GREEN}Donusum baslatiliyor...${NC}"
  echo "Dosya : $input_path"
  if [[ "$draw" == "1" ]]; then
    if [[ "$logy" == "1" ]]; then
      echo "Cizim  : logaritmik Y"
    else
      echo "Cizim  : lineer Y"
    fi
  else
    echo "Cizim  : kapali"
  fi
  echo

  # NOT: heredoc (<<EOF) kullanmiyoruz.
  # Heredoc bitince ROOT stdin kapanir, program cikar ve canvas kaybolur.
  # Cizim isteniyorsa ROOT etkilesimli kalmali (-q olmadan).
  if [[ "$draw" == "1" ]]; then
    echo -e "${YELLOW}Spektrum penceresi acik kalacak.${NC}"
    echo "Pencereyi inceledikten sonra ROOT'ta su komutu yazip Enter'a basin:"
    echo -e "  ${BOLD}.q${NC}"
    echo
    root -l \
      -e ".L universal_spectrum_to_root.C+" \
      -e "universal_spectrum_to_root(\"${escaped}\", \"\", ${draw_cpp}, ${force_cpp}, ${logy_cpp});" \
      -e "cout << endl << \"Spektrum acik. Cikmak icin: .q\" << endl;"
  else
    root -l -q \
      -e ".L universal_spectrum_to_root.C+" \
      -e "universal_spectrum_to_root(\"${escaped}\", \"\", ${draw_cpp}, ${force_cpp}, ${logy_cpp});"
  fi
}

run_root_batch() {
  local dir_path="$1"
  local force="$2"

  local force_cpp="false"
  [[ "$force" == "1" ]] && force_cpp="true"

  local escaped
  escaped=$(printf '%s' "$dir_path" | sed 's/\\/\\\\/g; s/"/\\"/g')

  echo
  echo -e "${GREEN}Toplu donusum baslatiliyor...${NC}"
  echo "Klasor : $dir_path"
  echo

  root -l -q <<EOF
.L universal_spectrum_to_root.C+
universal_spectrum_batch("${escaped}", false, ${force_cpp});
EOF
}

option_single() {
  echo
  echo -e "${BOLD}Tek dosya donusumu${NC}"
  echo "Tam yolu yapistirin (ornek:"
  echo "  /mnt/c/Users/PC/Desktop/spe/Eu152.spe"
  echo "  samples/ortec_ascii.spe"
  echo ")"
  read -r -p "Dosya yolu: " path || true
  path="$(strip_quotes "$path")"

  if [[ -z "$path" ]]; then
    echo -e "${RED}Dosya yolu bos.${NC}"
    return
  fi
  if [[ ! -f "$path" ]]; then
    echo -e "${RED}Dosya bulunamadi:${NC} $path"
    return
  fi

  local draw="1"
  local logy="1"
  local force="0"
  if ask_yes_no "Spektrumu ekranda cizmek ister misiniz?" "e"; then
    draw="1"
    echo
    echo "  E = logaritmik Y ekseni (kucuk ve buyuk pikler birlikte gorunur)"
    echo "  h = lineer Y ekseni"
    if ask_yes_no "Logaritmik olcek (log-Y) kullanilsin mi?" "e"; then
      logy="1"
    else
      logy="0"
    fi
  else
    draw="0"
    logy="0"
  fi

  if ask_yes_no "Bilinmeyen formatta zorla generic denensin mi? (genelde HAYIR)" "h"; then
    force="1"
    echo -e "${YELLOW}Uyari: forceGeneric risklidir; sadece bilerek kullanin.${NC}"
  fi

  run_root_single "$path" "$draw" "$force" "$logy"
}

option_batch() {
  echo
  echo -e "${BOLD}Klasor (toplu) donusumu${NC}"
  echo "Klasor yolunu yazin. Icindeki tum .spe ve .chn dosyalari donusturulur."
  read -r -p "Klasor yolu: " path || true
  path="$(strip_quotes "$path")"

  if [[ -z "$path" ]]; then
    echo -e "${RED}Klasor yolu bos.${NC}"
    return
  fi
  if [[ ! -d "$path" ]]; then
    echo -e "${RED}Klasor bulunamadi:${NC} $path"
    return
  fi

  local force="0"
  if ask_yes_no "Bilinmeyen formatta zorla generic denensin mi? (genelde HAYIR)" "h"; then
    force="1"
  fi

  run_root_batch "$path" "$force"
}

option_samples() {
  echo
  echo -e "${BOLD}Ornek dosyalar${NC}"
  echo "  a) samples/ortec_ascii.spe   (ASCII SPE)"
  echo "  b) samples/gf3_like.spe      (GF3-like binary SPE)"
  echo "  c) samples/ortec_demo.chn    (Ortec CHN)"
  echo "  d) Hepsini toplu donustur"
  read -r -p "Secim [a/b/c/d]: " choice || true
  choice="$(echo "$choice" | tr '[:upper:]' '[:lower:]')"

  case "$choice" in
    a) run_root_single "samples/ortec_ascii.spe" "1" "0" "1" ;;
    b) run_root_single "samples/gf3_like.spe" "1" "0" "1" ;;
    c) run_root_single "samples/ortec_demo.chn" "1" "0" "1" ;;
    d) run_root_batch "samples" "0" ;;
    *) echo -e "${RED}Gecersiz secim.${NC}" ;;
  esac
}

option_help() {
  echo
  echo -e "${BOLD}Bu arac ne yapar?${NC}"
  echo "ROOT'un dogrudan acamadigi .spe / .chn spektrum dosyalarini"
  echo "okur ve analiz icin .root dosyasina cevirir."
  echo
  echo -e "${BOLD}Temel fikir${NC}"
  echo "  dosyayi ham oku -> formati tani -> counts cikar -> ROOT'a yaz"
  echo
  echo -e "${BOLD}Desteklenenler${NC}"
  echo "  - Ortec/Maestro ASCII SPE"
  echo "  - GF3-like binary SPE"
  echo "  - Ortec binary CHN"
  echo
  echo -e "${BOLD}Cikti${NC}"
  echo "  Ayni isimde .root dosyasi + spektrum histogrami"
  echo
  echo -e "${BOLD}Ileri kullanim (ROOT bilenler)${NC}"
  echo "  root -l"
  echo "  .L universal_spectrum_to_root.C+"
  echo "  universal_spectrum_to_root(\"dosya.spe\");"
  echo
  echo -e "${BOLD}Sunum${NC}"
  echo "  docs/presentation/ust_sunum.pdf"
  echo
}

require_root
print_header

while true; do
  print_menu
  read -r -p "Seciminiz [1-5]: " choice || true
  case "$choice" in
    1) option_single ;;
    2) option_batch ;;
    3) option_samples ;;
    4) option_help ;;
    5)
      echo
      echo "Gule gule."
      exit 0
      ;;
    *)
      echo -e "${RED}Lutfen 1-5 arasi bir secim yapin.${NC}"
      ;;
  esac
  echo
  read -r -p "Menuye donmek icin Enter..." _ || true
  print_header
done
