
.SUFFIXES: .fig .eps

FIGS=\
	dsc-arch.eps

DOC=dsc-manual

$(DOC).pdf: $(DOC).ps
	ps2pdf $(DOC).ps > $@

$(DOC).ps: $(DOC).dvi
	dvips $(DOC).dvi > $@

$(DOC).dvi: $(DOC).tex $(FIGS)
	latex $(DOC).tex

.fig.eps:
	fig2dev -L eps $< > $@

clean:
	rm -f $(FIGS)
	rm -f $(DOC).aux
	rm -f $(DOC).dvi
	rm -f $(DOC).log
	rm -f $(DOC).pdf
	rm -f $(DOC).ps
