
export EMUIIBO_MAJOR := 0
export EMUIIBO_MINOR := 5
export EMUIIBO_MICRO := 0

.PHONY: all dev clean

base:
	@$(MAKE) -C libstratosphere/
	@$(MAKE) -C emuiibo/
	@$(MAKE) -C overlay/
	@rm -rf $(CURDIR)/SdOut
	@mkdir -p $(CURDIR)/SdOut/atmosphere/contents/0100000000000352/flags
	@touch $(CURDIR)/SdOut/atmosphere/contents/0100000000000352/flags/boot2.flag
	@cp $(CURDIR)/emuiibo/emuiibo.nsp $(CURDIR)/SdOut/atmosphere/contents/0100000000000352/exefs.nsp
	@mkdir -p $(CURDIR)/SdOut/switch/.overlays
	@cp $(CURDIR)/overlay/emuiibo.ovl $(CURDIR)/SdOut/switch/.overlays/emuiibo.ovl

setdev:
	$(eval export EMUIIBO_DEV := true)
	@echo
	@echo WARNING - Building in dev mode! use this at your own risk...
	@echo

nodev:
	$(eval export EMUIIBO_DEV := false)

all: nodev base
dev: setdev base

clean:
	@rm -rf $(CURDIR)/SdOut
	@$(MAKE) clean -C libstratosphere/
	@$(MAKE) clean -C emuiibo/
	@$(MAKE) clean -C overlay/