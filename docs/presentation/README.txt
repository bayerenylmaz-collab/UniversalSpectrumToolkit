Universal Spectrum Toolkit - Sunum
=================================

Dosyalar
--------
- ust_sunum.tex      : Beamer kaynak (Turkce)
- ust_sunum.pdf      : Derlenmis sunum (Turkce)
- ust_sunum_en.tex   : Beamer kaynak (English)
- ust_sunum_en.pdf   : Compiled presentation (English)
- user_shots/        : Gercek ekran goruntuleri
- figures/           : Eski sema gorselleri (arsiv)

PDF'i yeniden derlemek
----------------------
  cd docs/presentation
  pdflatex ust_sunum.tex && pdflatex ust_sunum.tex
  pdflatex ust_sunum_en.tex && pdflatex ust_sunum_en.tex

Icerik ozeti
------------
1. Sorun: ROOT .spe/.chn'i native spektrum olarak bilmez
2. Neden: dosyayi acmak != spektrumu dogru okumak
3. Tasarim: cevirici katmani (oku -> tani -> counts -> ROOT)
4. Mimari: moduler reader'lar
5. Once/sonra karsilastirma
6. Sonraki adim: menulu kullanici dostu arayuz
