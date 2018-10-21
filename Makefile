# Project folders that contain ipp subsystems
PROJECTS := $(shell find ???-* -name Makefile)

%.ipp_build :
	+@$(MAKE) -C $(dir $*)

%.ipp_clean : 
	+@$(MAKE) -C $(dir $*) clean

%.ipp_clobber :
	+@$(MAKE) -C $(dir $*) clobber

%.ipp_tidy :
	+@$(MAKE) -C $(dir $*) tidy

%.ipp_install :
	+@$(MAKE) -C $(dir $*) install

all:	$(addsuffix .ipp_build,$(PROJECTS))
	@echo "Finished building IPP"

build:	$(addsuffix .ipp_build,$(PROJECTS))

clean:	$(addsuffix .ipp_clean,$(PROJECTS))

clobber: $(addsuffix .ipp_clobber,$(PROJECTS))

tidy:	$(addsuffix .ipp_tidy,$(PROJECTS))

install: $(addsuffix .ipp_install,$(PROJECTS))
