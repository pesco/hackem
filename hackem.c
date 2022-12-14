/* hackem.c - an emulator for the Hack computer featuring (paper) tape I/O
 * pesco 2022
 *
 * Cf. Nisan and Schocken, The Elements of Computing Systems, MIT Press.
 */

#include <stdio.h>	/* FILE */
#include <stdint.h>	/* uint16_t */


/*
 * Memory-mapped devices.
 *
 * We will map a tape reader and a tape punch to stdin/stdout, respectively,
 * plus an auxiliary printer to stderr.
 * We reserve space for screen and keyboard, but leave their functionality
 * unimplemented for now.
 */

struct device {
	void (*rfun)(struct device *, uint16_t *, uint16_t);
	void (*wfun)(struct device *, uint16_t *, uint16_t);
	void (*tfun)(struct device *, uint16_t *);
};

struct tape {
	struct device dev;
	FILE *stream;
	long busy;
};

/* I/O update routines */
void w_dummy(struct device *, uint16_t *, uint16_t);
void w_itape(struct device *, uint16_t *, uint16_t);
void t_itape(struct device *, uint16_t *);
void r_otape(struct device *, uint16_t *, uint16_t);
void w_otape(struct device *, uint16_t *, uint16_t);
void t_otape(struct device *, uint16_t *);

/* device structures */
struct device dev_dummy	= {NULL, w_dummy};	/* writes are no-ops */
struct tape dev_itape	= {{NULL, w_itape, t_itape}, stdin};
struct tape dev_otape	= {{r_otape, w_otape, t_otape}, stdout};
struct tape dev_printer	= {{r_otape, w_otape, t_otape}, stderr};

/* instruction and data memories */
uint16_t ram[32 * 1024];			/* includes I/O space */
uint16_t *rom;					/* mmapped from file */

/* memory map */
#define DEV(X)	((struct device *) &(dev_ ## X))
#define MASK(N)	(~0 << (N))			/* low N bits zero */
struct mem {
	uint16_t base, mask;
	struct device *dev;
} memmap[] = {
	{0x0000, MASK(14)},			/* RAM */
	{0x4000, MASK(12)},			/* screen */
	{0x6000, MASK(0),	DEV(dummy)},	/* keyboard */
	{0x7000, MASK(1),	DEV(itape)},	/* tape reader */
	{0x7002, MASK(1),	DEV(otape)},	/* tape punch */
	{0x7004, MASK(1),	DEV(printer)},	/* printer */
};
#define MAPSIZE (sizeof memmap / sizeof(struct mem))

/* find an address's entry in the memory map */
struct mem *
lookup(uint16_t addr)
{
	struct mem *p;

	for (p = memmap; p < memmap + MAPSIZE; p++)
		if ((addr & p->mask) == p->base)
			return p;

	return NULL;				/* address not mapped */
}

/* call every device's tick function, if present */
void
device_ticks(void)
{
	struct mem *p;

	for (p = memmap; p < memmap + MAPSIZE; p++)
		if (p->dev != NULL && p->dev->tfun != NULL)
			p->dev->tfun(p->dev, ram + p->base);
}

/* memory accessor: read operations */
uint16_t
readmem(uint16_t address)
{
	struct mem *p;

	/* find address in memory map, return 0 for unmapped address space */
	if ((p = lookup(address)) == NULL)
		return 0;

	/* call device input function if one is present */
	if (p->dev != NULL && p->dev->rfun != NULL)
		p->dev->rfun(p->dev, ram + p->base, address & ~p->mask);

	/* perform the actual read */
	return ram[address];
}

/* memory accessor: write operations */
void
writemem(uint16_t address, uint16_t value)
{
	struct mem *p;
	
	/* find address in memory map, do nothing on unmapped address space */
	if ((p = lookup(address)) == NULL)
		return;

	/* perform the actual write */
	ram[address] = value;

	/* call device output function if one is present */
	if (p->dev != NULL && p->dev->wfun != NULL)
		p->dev->wfun(p->dev, ram + p->base, address & ~p->mask);
}


/*
 * Arithmetic/Logic Unit.
 */

/* helpers to extract one or more bits from a word */
#define bit(x, p)	(((x) >> (p)) & 1)
#define bits(x, p, n)	(((x) >> (p)) & ~(~0 << (n)))

/* derived helpers */
#define positive(x)	(bit(x, 15) == 0 && (x) != 0)
#define negative(x)	(bit(x, 15) == 1)

/* conversion to signed integer */
#define sint(x)		(negative(x) ? (x) - (1 << 16) : (x))

/* comp = zx nx zy ny f no */
uint16_t
alu(uint16_t x, uint16_t y, uint16_t comp)
{
	uint16_t out;

	if (bit(comp, 5))	/* zx: zero x */
		x = 0;
	if (bit(comp, 4))	/* nx: negate x */
		x = ~x;
	if (bit(comp, 3))	/* zy: zero y */
		y = 0;
	if (bit(comp, 2))	/* ny: negate y */
		y = ~y;

	if (bit(comp, 1))	/* f */
		out = x + y;
	else
		out = x & y;

	if (bit(comp, 0))	/* no: negate output */
		out = ~out;

	return out;
}


/*
 * Central Processing Unit.
 */

uint16_t A, D, PC;			/* registers */

int
cpu(uint16_t instr)
{
	uint16_t a, comp, ddd, jjj;	/* instruction parts */
	uint16_t dest, result, y;

	if (bit(instr, 15)) {		/* C-instruction */
		/* decode instruction */
		jjj	= bits(instr, 0, 3);
		ddd	= bits(instr, 3, 3);
		comp	= bits(instr, 6, 6);
		a	= bits(instr, 12, 1);

		/* save jump destination before updating A */
		dest	= A;

		/* switch ALU input between A and M */
		if (a)
			y = readmem(A);
		else
			y = A;

		/* perform computation */
		result = alu(D, y, comp);

		/* distribute result to destinations */
		if (bit(ddd, 0))	/* M */
			writemem(A, result);
		if (bit(ddd, 1))	/* D */
			D = result;
		if (bit(ddd, 2))	/* A */
			A = result;

		/*
		 * Terminate on idiomatic infinite loops. I.e. an
		 * unconditional jump without any assignment that leads
		 * (a) directly to itself or (b) to an immediately
		 * preceding instruction that loads its own address.
		 *
		 * 	(a)  @END	(b)  (END)
		 *	     (END)	     @END
		 *	     0;JMP	     0;JMP
		 */
		if (ddd == 0 && jjj == 7 && 
		    (dest == PC || (dest == PC - 1 && rom[dest] == dest)))
			return 0;	/* stop */

		/* perform jump if required */
		if ((bit(jjj, 0) && positive(result)) ||
		    (bit(jjj, 1) && result == 0) ||
		    (bit(jjj, 2) && negative(result)))
			PC = dest - 1;
	} else				/* A-instruction */
		A = instr;
	PC++;

	device_ticks();			/* time-based I/O updates */
	return 1;			/* keep running */
}


/*
 * Main program.
 */

#include <assert.h>
#include <errno.h>
#include <inttypes.h>	/* PRI... */
#include <math.h>	/* modf */
#include <stdlib.h>	/* exit, strtol, strtod */
#include <time.h>	/* clock_gettime, nanosleep */
#include <fcntl.h>	/* open */
#include <unistd.h>	/* close, getopt */
#include <sys/mman.h>	/* mmap */
#include <sys/time.h>	/* timespecsub, timespecadd, timespeccmp */
#include <err.h>

long T;			/* global clock tick counter */

/* command line options */
FILE *tfile;
long cpufreq = 1000;	/* clock frequency [Hz] */
double sfactor = 1.0;	/* time scale (0 = fastest, 1 = realtime, ...) */

void
tracehdr(void)
{
	if (tfile == NULL)
		return;

	fprintf(tfile, "T\tPC\tinstr.\tA\tD\t"
	    "R0/SP\tR1/LCL\tR2/ARG\tR3/THIS\tR4/THAT\n");
}

void
trace(uint16_t PC, uint16_t instr, uint16_t A, uint16_t D)
{
	int i;

	if (tfile == NULL)
		return;

	fprintf(tfile, "%ld\t%" PRIu16 "\t%.6" PRIo16 "\t%d\t%d", T, PC, instr,
	    sint(A), sint(D));
	for (i = 0; i < 5; i++)
		fprintf(tfile, "\t%d", sint(ram[i]));
	putc('\n', tfile);
}

void
usage(void)
{
	extern char *__progname;

	fprintf(stderr, "usage: %s [-f freq] [-s factor] [-t file] prog.rom\n",
	    __progname);
	exit(100);
}

int
main(int argc, char *argv[])
{
	struct timespec t, t1, dur;
	double d;
	char *ep;
	int fd, c;
	uint16_t instr;

	/* process command line flags */
	while ((c = getopt(argc, argv, "f:s:t:")) != -1) {
		switch (c) {
		case 'f':
			cpufreq = strtol(optarg, &ep, 10);
			if (cpufreq <= 0 || *ep != '\0')
				errx(100, "-f: invalid argument");
			break;
		case 's':
			sfactor = strtod(optarg, &ep);
			if (sfactor < 0 || *ep != '\0')
				errx(100, "-s: invalid argument");
			break;
		case 't':
			if ((tfile = fopen(optarg, "w")) == NULL)
				err(100, "%s", optarg);
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	/* map rom file into memory */
	if (argc < 1)
		usage();
	if ((fd = open(argv[0], O_RDONLY)) == -1)
		err(100, "open");
	rom = mmap(NULL, 0x8000, PROT_READ, MAP_PRIVATE, fd, 0);
	if (rom == MAP_FAILED)
		err(100, "mmap");
	close(fd);

	/* print header line to trace file, if applicable */
	tracehdr();

	/* disable stdout buffering unless running at max speed */
	if (sfactor > 0.0)
		setbuf(stdout, NULL);

	/* compute the desired duration of one clock tick */
	dur.tv_nsec = 1000000000L * modf(sfactor / cpufreq, &d);
	dur.tv_sec = d;

	/* initialization */
	A = D = PC = 0;
	clock_gettime(CLOCK_MONOTONIC, &t1);

	/* the main loop */
	for (T = 0; ; T++) {
		instr = rom[PC];		/* fetch instruction */
		trace(PC, instr, A, D);		/* output cur. CPU state */
		if (!cpu(instr))		/* execute instruction */
			break;			/* terminate */

		/* wait out the rest of the tick */
		timespecadd(&t1, &dur, &t1);	/* when is the next tick? */
		clock_gettime(CLOCK_MONOTONIC, &t);
		if (timespeccmp(&t, &t1, <)) {	/* is that in the future? */
			timespecsub(&t1, &t, &t);
			while (nanosleep(&t, &t) == -1)
				assert(errno == EINTR);
		}
	}

	/* always return success in trace mode */
	if (tfile)
		return 0;

	return D;
}


/*
 * The dummy device ignores writes (all memory cells stay at value zero).
 */

void
w_dummy(struct device *dev, uint16_t *mem, uint16_t offset)
{
	mem[offset] = 0;	/* nullify any previous write */
}


/*
 * The tape reader provides two addresses.
 *
 * Offset 0 reflects the current value under the read head. It is well-defined
 * only when the control word at offset 1 indicates a valid position. A negative
 * value indicates the end of the tape (or no tape present at all).
 * Writing to offset 0 has no effect.
 *
 * Offset 1 provides control over the tape feed. Writing 1 to it will advance
 * the tape by one position. The result of writing any other value is undefined.
 * Reading from offset 1 yields a status word. A negative value (bit 15 set)
 * indicates that the value at offset 0 is valid. All other bits are undefined.
 *
 * Procedure to acquire one byte of input:
 *   1.  Write 1 to offset 1.
 *   2.  Read offset 1 until it is negative.
 *   3.  Read offset 0; if it is negative, indicate the end of input.
 *   4.  Otherwise, the value read from offset 0 is the input byte.
 */

void
w_itape(struct device *dev, uint16_t *mem, uint16_t offset)
{
	struct tape *tp = (struct tape *)dev;

	if (offset & 1) {			/* offset 1 */
		if (tp->busy)
			return;

		/* store "garbage" at offset 0 and "invalid" at offset 1 */
		mem[0] = 0;
		mem[1] = 0;

		/* mark drive busy for 150 ms */
		tp->busy = cpufreq * 150 / 1000;
		assert(tp->busy >= 0);
	} else {				/* offset 0 */
		/* no effect */
	}
}

void
t_itape(struct device *dev, uint16_t *mem)
{
	struct tape *tp = (struct tape *)dev;
	int c;

	if (tp->busy == 0)			/* idle */
		return;

	if (--tp->busy == 0) {
		/* fetch and store the next character */
		if ((c = fgetc(tp->stream)) != EOF)
			mem[0] = c;
		else
			mem[0] = 0x8000;

		/* set control word to "ready" */
		mem[1] = 0x8000;
	}
}


/*
 * The tape punch provides two addresses.
 *
 * A write to offset 0 causes the corresponding byte to be output and the tape
 * advanced automatically to the next position. The result is only well-defined
 * if the status word at offset 1 indicates readiness.
 * The result of reading from offset 0 is undefined.
 *
 * Reading from offset 1 yields a status word. A negative value (bit 15 set)
 * indicates that the tape is in position for new output. This is invalid,
 * however, if 14 is also set. Bit 14 indicates the end of the tape (or that no
 * tape is present). All other bits are undefined.
 * The result of writing to offset 1 is undefined.
 * 
 * Procedure to produce one byte of output:
 *   1.  Read offset 1 until it is negative.
 *   2.  If bit 14 is set, indicate an error.
 *   3.  Write the output byte to offset 0.
 */

void
r_otape(struct device *dev, uint16_t *mem, uint16_t offset)
{
	struct tape *tp = (struct tape *)dev;
	uint16_t w = 0;

	if (offset & 1) {			/* offset 1 */
		/* bit 15: tape position */
		if (!tp->busy)
			w |= 0x8000u;

		/* bit 14: tape presence */
		if (feof(tp->stream))
			w |= 0x4000u;
	} else {				/* offset 0 */
		/* bit 15: tape presence */
		if (feof(tp->stream))
			w |= 0x8000u;
	}

	mem[offset] = w;
}

void
w_otape(struct device *dev, uint16_t *mem, uint16_t offset)
{
	struct tape *tp = (struct tape *)dev;

	if (offset & 1) {			/* offset 1 */
		/* no effect */
	} else {				/* offset 0 */
		/* no effect if tape not ready */
		if (tp->busy)
			return;

		/* NB: no effect if stream at EOF */
		fputc((unsigned char) mem[offset], tp->stream);

		/* set tape busy for 150 ms */
		tp->busy = cpufreq * 150 / 1000;
		assert(tp->busy >= 0);
	}
}

void
t_otape(struct device *dev, uint16_t *mem)
{
	struct tape *tp = (struct tape *)dev;

	if (tp->busy > 0)
		tp->busy--;
}
