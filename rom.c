/* rom.c - convert a .hack file to a ROM image for the Hack emulator
 * pesco 2022
 *
 * Opens a (non-portable) .rom image file for writing and reads an ASCII
 * representation of its desired contents from standard input in .hack format.
 *
 * Accepts a relaxed superset of the plain text .hack file format as described
 * in Chapter 6 of The Elements of Computing Systems by Nisan and Schocken.
 * 
 * Every line in the input consists of sixteen ASCII binary digits ('0' or '1'),
 * representing one word of memory. Each line must contain exactly 16
 * digits. Horizontal whitespace and blank lines are ignored. A hash '#'
 * character introduces a comment - the rest of the line is ignored.
 *
 * Every input word is written to the rom file in machine byte order,
 * suitable for being mmapped directly by the emulator.
 */

#include <stdio.h>
#include <stdlib.h>	/* exit */
#include <stdint.h>	/* uint16_t */
#include <ctype.h>	/* isspace */
#include <err.h>

extern char *__progname;

void
usage(void)
{
	fprintf(stderr, "usage: %s prog.rom < prog.hack\n", __progname);
	exit(1);
}

int
main(int argc, char **argv)
{
	FILE *fout;
	int c, n, line;
	uint16_t x;

	// XXX getopt
	--argc;
	if (argc < 1)
		usage();

	/* open output file for writing */
	if ((fout = fopen(argv[1], "wb")) == NULL)
		err(1, "%s", argv[1]);

	/* main loop */
	line = 1;
	n = 0;
	x = 0;
	while ((c = getchar()) != EOF) {
		if (c == '#') {			/* comment */
			while ((c = getchar()) != EOF && c != '\n')
				;
			if (c == EOF)
				break;
		}

		/* detect and handle the end of the line */
		if (c == '\n') {
			if (n != 16 && n != 0)	/* wrong number of digits */
				break;

			if (n != 0)		/* ignore empty lines */
				if (fwrite(&x, sizeof x, 1, fout) == 0)
					err(1, "%s: write error", argv[1]);

			line++;			/* process next line... */
			n = 0;
			continue;
		}

		if (isspace(c))			/* horizontal whitespace */
			continue;

		if (c != '0' && c != '1')	/* unexpected character */
			errx(1, "line %d: expected '0' or '1'", line);

		x = x << 1 | (c - '0');		/* read one digit into x */
		n++;
	}
	if (ferror(stdin))
		err(1, "%s", __progname);

	if (n == 16)				/* final line had no '\n' */
		fwrite(&x, sizeof x, 1, fout);
	else if (n != 0)
		errx(1, "line %d: wrong number of digits (must be 16)", line);

	return 0;				/* completed successfully */
}
