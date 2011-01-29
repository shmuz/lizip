# Documentation Makefile

APP = rst2html.py
CSS = --stylesheet-path=lrexlib.css --link-stylesheet
HDR = --initial-header-level=2
DT  = --date --time
#GT  = -gt

%.html : %.txt
	$(APP) $(CSS) $(GT) $(HDR) $(DT) $< $@

all: manual.html
