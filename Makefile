.PHONY: all roms
all: hackem rom roms
roms: add.rom term0.rom term1.rom term2.rom

.SUFFIXES: .hack .rom .tsv
.hack.rom:
	./rom $@ < $<
.rom.tsv:
	./hackem -t $@ $<
