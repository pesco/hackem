PROGS = hackem rom
ROMS = add.rom term0.rom term1.rom

all: ${PROGS} ${ROMS}

.SUFFIXES: .hack .rom
.hack.rom:
	./rom $@ < $<
