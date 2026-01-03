/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* sdl-coord-plane-option-parser.h */
/* Copyright (C) 2020-2026 Eric Herman <eric@freesa.org> */

#ifndef COORD_PLANE_OPTION_PARSER_H
#define COORD_PLANE_OPTION_PARSER_H 1

#include "coord-plane-iteration.h"

#include <stdio.h>

void print_help(FILE *out, const char *argv0, const char *version);
coordinate_plane_s *coordinate_plane_new_from_args(int argc, char **argv,
						   const char *version);
void print_command_line(coordinate_plane_s *plane, FILE *out);

#endif /* COORD_PLANE_OPTION_PARSER_H */
