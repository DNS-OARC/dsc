
.SUFFIXES: .fig .eps .png

FIGS=\
	dsc-arch.eps \
	screenshot1.eps

DOC=dsc-manual

$(DOC).pdf: $(DOC).ps
	ps2pdf $(DOC).ps > $@

$(DOC).ps: $(DOC).dvi
	dvips $(DOC).dvi > $@

$(DOC).dvi: $(DOC).tex $(FIGS)
	latex $(DOC).tex
	latex $(DOC).tex
	latex $(DOC).tex

.fig.eps:
	fig2dev -L eps $< > $@

.png.eps:
	pngtopnm $< | pnmtops -noturn > $@
	

all: $(DOC).ps $(DOC).pdf

clean-junk:
	rm -f $(FIGS)
	rm -f $(DOC).aux
	rm -f $(DOC).dvi
	rm -f $(DOC).log
	rm -f $(DOC).toc

clean: clean-junk
	rm -f $(DOC).pdf
	rm -f $(DOC).ps

clean-release: clean-junk
	rm -f $(DOC).tex
	rm -f dsc-arch.fig
	rm -f screenshot1.png
	rm -f Makefile
