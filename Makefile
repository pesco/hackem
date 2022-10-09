CFLAGS += -Wall

PROGS = hackem rom
ROMS = add.rom max.rom term0.rom term1.rom term2.rom dest.rom hello.rom atoi.rom

.PHONY: all test clean
all: ${PROGS} ${ROMS}
clean:
	rm -f ${PROGS} ${ROMS}
test: all
	@sh test.sh ${ROMS}

.SUFFIXES: .hack .rom .tsv
.hack.rom:
	./rom $@ < $<
.rom.tsv:
	./hackem -t $@ $<
