/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* coord-plane-option-parser.c */
/* Copyright (C) 2020-2026 Eric Herman <eric@freesa.org> */

#include <coord-plane-option-parser.h>
#include <math.h>
#include <stdlib.h>
#include <float.h>
#include <getopt.h>
#include <inttypes.h>

#ifndef SKIP_THREADS
#include <unistd.h>
#endif

void print_command_line(coordinate_plane_s *plane, FILE *out)
{
	const char *argv0 = coordinate_plane_program(plane);
	size_t pfuncs_idx = coordinate_plane_function_index(plane);

	fprintf(out, "%s --function=%zu", argv0, pfuncs_idx);
	if (pfuncs_idx == pfuncs_julia_idx) {
		ldxy_s seed;
		coordinate_plane_seed(plane, &seed);
		fprintf(out, " --seed_x=%.*Lg --seed_y=%.*Lg", DECIMAL_DIG,
			seed.x, DECIMAL_DIG, seed.y);
	}
	uint32_t skip_rounds = coordinate_plane_skip_rounds(plane);
	if (skip_rounds) {
		fprintf(out, " --skip_rounds=%" PRIu32, skip_rounds);
	}
	ldxy_s center;
	coordinate_plane_center(plane, &center);
	fprintf(out, " --center_x=%.*Lg --center_y=%.*Lg", DECIMAL_DIG,
		center.x, DECIMAL_DIG, center.y);
	long double x_min = coordinate_plane_x_min(plane);
	long double x_max = coordinate_plane_x_max(plane);
	fprintf(out, " --from=%.*Lg --to=%.*Lg", DECIMAL_DIG, x_min,
		DECIMAL_DIG, x_max);
	uint32_t win_width = coordinate_plane_win_width(plane);
	fprintf(out, " --width=%" PRIu32, win_width);
	uint32_t win_height = coordinate_plane_win_height(plane);
	fprintf(out, " --height=%" PRIu32, win_height);
	fprintf(out, "\n");
	long double y_min = coordinate_plane_y_min(plane);
	long double y_max = coordinate_plane_y_max(plane);
	fprintf(out,
		"(y-axis co-ordinates range from: %.*Lg to: %.*Lg)\n",
		DECIMAL_DIG, y_min, DECIMAL_DIG, y_max);
}

typedef struct coord_options {
	int win_width;
	int win_height;
	long double x_min;
	long double x_max;
	long double y_min;
	long double y_max;
	long double center_x;
	long double center_y;
	long double seed_x;
	long double seed_y;
	int function;
	int threads;
	int halt_after;
	int skip_rounds;
	int version;
	int help;
} coord_options_s;

static void coord_options_init(coord_options_s *options)
{
	options->win_width = -1;
	options->win_height = -1;
	options->x_min = NAN;
	options->x_max = NAN;
	options->y_min = NAN;
	options->y_max = NAN;
	options->center_x = NAN;
	options->center_y = NAN;
	options->function = -1;
	options->seed_x = NAN;
	options->seed_y = NAN;
	options->threads = -1;
	options->halt_after = -1;
	options->skip_rounds = -1;
	options->version = 0;
	options->help = 0;
}

static void coord_options_rationalize(coord_options_s *options)
{
	if (options->help != 1) {
		options->help = 0;
	}
	if (options->version != 1) {
		options->version = 0;
	}
	if (options->win_width < 1) {
#ifndef NO_GUI
		options->win_width = 800;
#else
		options->win_width = 79;
#endif
	}
	if (options->win_height < 1) {
#ifndef NO_GUI
		options->win_height = ((options->win_width * 3) / 4);
#else
		options->win_height = 24;
#endif
	}
	if (!isfinite(options->x_min)) {
		options->x_min = -2.5;
	}
	if (!isfinite(options->x_max)) {
		options->x_max = options->x_min + 4.0;
	}
	if (!isfinite(options->y_min)) {
		double xy_ratio =
		    ((1.0 * options->win_height) / options->win_width);
		double x_range = fabsl(options->x_max - options->x_min);
		double y_range = (x_range * xy_ratio);
		options->y_min = -(fmax(1.5, y_range / 2));
	}
	if (!isfinite(options->y_max)) {
		options->y_max = -(options->y_min);
	}
	if (!isfinite(options->center_x)) {
		options->center_x = -0.5;
	}
	if (!isfinite(options->center_y)) {
		options->center_y = 0.0;
	}
	if (options->function < 0
	    || (((size_t)options->function) >= pfuncs_len)) {
		options->function = pfuncs_mandlebrot_idx;
	}
	if (!isfinite(options->seed_x)) {
		options->seed_x = -1.25643;
	}
	if (!isfinite(options->seed_y)) {
		options->seed_y = -0.381086;
	}
	if (options->halt_after < 0) {
		options->halt_after = 0;
	}
	if (options->skip_rounds < 0) {
		options->skip_rounds = 0;
	}
	if (options->threads < 1) {
#ifndef SKIP_THREADS
		options->threads = (uint32_t)sysconf(_SC_NPROCESSORS_ONLN);
		if (options->threads > 1) {
			--(options->threads);
		}
#else
		options->threads = 1;
#endif
	}
}

static int coord_options_parse_argv(coord_options_s *options,
				    int argc, char **argv, FILE *err)
{
	int opt_char;
	int option_index;

	/* yes, optstirng is horrible */
	const char *optstring = "HVw:h:x:y:f:t:j:r:i:c:a:s:";

	struct option long_options[] = {
		{ "help", no_argument, 0, 'H' },
		{ "version", no_argument, 0, 'V' },
		{ "width", required_argument, 0, 'w' },
		{ "height", required_argument, 0, 'h' },
		{ "center_x", required_argument, 0, 'x' },
		{ "center_y", required_argument, 0, 'y' },
		{ "from", required_argument, 0, 'f' },
		{ "to", required_argument, 0, 't' },
		{ "function", required_argument, 0, 'j' },
		{ "seed_x", required_argument, 0, 'r' },
		{ "seed_y", required_argument, 0, 'i' },
		{ "threads", required_argument, 0, 'c' },
		{ "halt_after", required_argument, 0, 'a' },
		{ "skip_rounds", required_argument, 0, 's' },
		{ 0, 0, 0, 0 }
	};

	while (1) {
		option_index = 0;

		opt_char =
		    getopt_long(argc, argv, optstring, long_options,
				&option_index);

		/* Detect the end of the options. */
		if (opt_char == -1) {
			break;
		}

		switch (opt_char) {
		case 0:
			break;
		case 'H':	/* --help | -H */
			options->help = 1;
			break;
		case 'V':	/* --version | -V */
			options->version = 1;
			break;
		case 'w':	/* --width | -w */
			options->win_width = atoi(optarg);
			break;
		case 'h':	/* --height | -h */
			options->win_height = atoi(optarg);
			break;
		case 'f':	/* --from | -f */
			options->x_min = strtold(optarg, NULL);
			break;
		case 't':	/* --to | -t */
			options->x_max = strtold(optarg, NULL);
			break;
		case 'x':	/* --center_x | -x */
			options->center_x = strtold(optarg, NULL);
			break;
		case 'y':	/* --center_y | -y */
			options->center_y = strtold(optarg, NULL);
			break;
		case 'j':	/* --function | -j */
			options->function = atoi(optarg);
			break;
		case 'r':	/* --seed_x | -r */
			options->seed_x = strtold(optarg, NULL);
			break;
		case 'i':	/* --seed_y | -i */
			options->seed_y = strtold(optarg, NULL);
			break;
		case 'c':	/* --threads | -c */
			options->threads = atoi(optarg);
			break;
		case 'a':	/* --halt_after | -a */
			options->halt_after = atoi(optarg);
			break;
		case 's':	/* --skip_rounds | -s */
			options->skip_rounds = atoi(optarg);
			break;
		default:
			options->help = 1;
			fprintf(err, "unrecognized option: '%c'\n", opt_char);
			fflush(err);
			break;
		}
	}
	return option_index;
}

void print_help(FILE *out, const char *argv0, const char *version)
{
	if (!out) {
		return;
	}
	fprintf(out, "%s version %s\n", argv0, version);
	fprintf(out, "OPTIONS:\n");
	fprintf(out, "\t-w --width=n       Witdth of the window in pixels\n");
	fprintf(out, "\t-h --height=n      Height of the window in pixels\n");
	fprintf(out, "\t-x --center_x=f    Center of the x-axis, '0.0'\n");
	fprintf(out, "\t-y --center_y=f    Center of the y-axis, '0.0'\n");
	fprintf(out, "\t-f --from=f        Left of the x-axis\n");
	fprintf(out, "\t                           default is '-2.5'\n");
	fprintf(out, "\t-t --to=f          Right of the x-axis\n");
	fprintf(out, "\t                           default is '1.5'\n");
	fprintf(out, "\t-j --function=n    Function number\n");
	fprintf(out, "\t                           0 for Mandlebrot\n");
	fprintf(out, "\t                           1 for Julia\n");
	fprintf(out, "\t-r --seed_x=f      Real (x) part of the Julia seed\n");
	fprintf(out, "\t-i --seed_y=f      Imaginary (y) part of the seed\n");
#ifndef SKIP_THREADS
	fprintf(out, "\t-c --threads=n     Number of threads/cores to use\n");
#else
	fprintf(out, "\t-c --threads=n     Not available: --DSKIP_THREADS\n");
#endif
	fprintf(out, "\t-a --halt_after=n  Execute this many iterations\n");
	fprintf(out, "\t-s --skip_rounds=n Number of iterations left black\n");
	fprintf(out, "\t-v --version       Print version and exit\n");
	fprintf(out, "\t-h --help          This message and exit\n");
}

coordinate_plane_s *coordinate_plane_new_from_args(int argc, char **argv,
						   const char *version)
{
	coord_options_s options;
	coord_options_init(&options);
	coord_options_parse_argv(&options, argc, argv, stdout);
	coord_options_rationalize(&options);

	if (options.help) {
		print_help(stdout, argv[0], version);
		exit(EXIT_SUCCESS);
	}

	if (options.version) {
		fprintf(stdout, "%s\n", version);
		exit(EXIT_SUCCESS);
	}

	ldxy_s seed = { options.seed_x, options.seed_y };
	ldxy_s center = { options.center_x, options.center_y };
	long double resolution_x =
	    (options.x_max - options.x_min) / (1.0 * options.win_width);
	long double resolution_y =
	    (options.y_max - options.y_min) / (1.0 * options.win_height);

	coordinate_plane_s *plane =
	    coordinate_plane_new(argv[0], options.win_width, options.win_height,
				 center, resolution_x, resolution_y,
				 options.function, seed,
				 options.halt_after, options.skip_rounds,
				 options.threads);

	return plane;
}
