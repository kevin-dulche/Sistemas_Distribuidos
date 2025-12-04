 #
 # $Id: Makefile,v 10.2 1997/12/24 06:58:25 alc Exp $
 #
all:  doc.ps install.ps install-demo.ps

txtfiles: install-demo.dvi install.dvi
	dvi2tty -w90 -o install-demo.txt install-demo.dvi 
	dvi2tty -w90 -o  install.txt install.dvi

doc.dvi: doc.tex
	latex doc.tex
	latex doc.tex

doc.ps: doc.dvi
	dvips doc -o

install.dvi: install.tex
	latex install.tex

install.ps: install.dvi
	dvips install -o

install-demo.dvi: install-demo.tex
	latex install-demo.tex

install-demo.ps: install-demo.dvi
	dvips install-demo -o

tidy:
	/bin/rm -f  *~ *.log *.aux *.toc

clean:
	/bin/rm -f *~ *.log *.aux *.dvi *.ps *.toc
