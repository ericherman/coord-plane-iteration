/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* rgb-hsv.c: colors */
/* Copyright (C) 2020-2026 Eric Herman <eric@freesa.org> */

#include <math.h>

#include "rgb-hsv.h"

int rgb24_from_rgb(rgb24_s *out, rgb_s in)
{
#ifndef NDEBUG
	if (!out) {
		return 1;
	}
#endif
	out->red = 255 * in.red;
	out->green = 255 * in.green;
	out->blue = 255 * in.blue;

	return 0;
}

int rgb24_from_uint32(rgb24_s *out, uint32_t in)
{
#ifndef NDEBUG
	if (!out) {
		return 1;
	}
#endif
	out->red = 0xFF & (in >> 16);
	out->green = 0xFF & (in >> 8);
	out->blue = 0xFF & (in);

	return 0;
}

uint32_t rgb24_to_uint32(rgb24_s rgb)
{
	uint32_t urgb = ((rgb.red << 16) + (rgb.green << 8) + rgb.blue);
	return urgb;
}

#ifndef NDEBUG
static int invalid_hsv_s(hsv_s hsv)
{
	if (!(hsv.hue >= 0.0 && hsv.hue <= 360.0)) {
		return 1;
	}
	if (!(hsv.sat >= 0 && hsv.sat <= 1.0)) {
		return 1;
	}
	if (!(hsv.val >= 0 && hsv.val <= 1.0)) {
		return 1;
	}
	return 0;
}
#endif

// https://dystopiancode.blogspot.com/2012/06/hsv-rgb-conversion-algorithms-in-c.html
// https://en.wikipedia.org/wiki/HSL_and_HSV
int rgb_from_hsv(rgb_s *rgb, hsv_s hsv)
{
#ifndef NDEBUG
	if (!rgb || invalid_hsv_s(hsv)) {
		return 1;
	}
#endif

	double hue = hsv.hue == 360.0 ? 0.0 : hsv.hue;
	double chroma = hsv.val * hsv.sat;
	double offset = chroma * (1.0 - fabs(fmod(hue / 60.0, 2) - 1.0));
	double smallm = hsv.val - chroma;

	if (hue >= 0.0 && hue < 60.0) {
		rgb->red = chroma + smallm;
		rgb->green = offset + smallm;
		rgb->blue = smallm;
	} else if (hue >= 60.0 && hue < 120.0) {
		rgb->red = offset + smallm;
		rgb->green = chroma + smallm;
		rgb->blue = smallm;
	} else if (hue >= 120.0 && hue < 180.0) {
		rgb->red = smallm;
		rgb->green = chroma + smallm;
		rgb->blue = offset + smallm;
	} else if (hue >= 180.0 && hue < 240.0) {
		rgb->red = smallm;
		rgb->green = offset + smallm;
		rgb->blue = chroma + smallm;
	} else if (hue >= 240.0 && hue < 300.0) {
		rgb->red = offset + smallm;
		rgb->green = smallm;
		rgb->blue = chroma + smallm;
	} else if (hue >= 300.0 && hue < 360.0) {
		rgb->red = chroma + smallm;
		rgb->green = smallm;
		rgb->blue = offset + smallm;
	} else {
		rgb->red = smallm;
		rgb->green = smallm;
		rgb->blue = smallm;
	}

	return 0;
}
