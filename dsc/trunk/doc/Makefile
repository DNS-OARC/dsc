
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
