/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* rgb-hsv.c: colors */
/* Copyright (C) 2020-2026 Eric Herman <eric@freesa.org> */

#include <assert.h>
#include <math.h>

#include "rgb-hsv.h"

int rgb_24_from_rgb_d(struct rgb_24 *out, struct rgb_d in)
{
	assert(out);

	out->red = 255 * in.red;
	out->green = 255 * in.green;
	out->blue = 255 * in.blue;

	return 0;
}

int rgb_24_from_uint32(struct rgb_24 *out, uint32_t in)
{

	assert(out);

	out->red = 0xFF & (in >> 16);
	out->green = 0xFF & (in >> 8);
	out->blue = 0xFF & (in);

	return 0;
}

uint32_t rgb_24_to_uint32(struct rgb_24 rgb)
{
	uint32_t urgb = ((rgb.red << 16) + (rgb.green << 8) + rgb.blue);
	return urgb;
}

int invalid_hsv(struct hsv_d hsv)
{
	if (!(hsv.hue >= 0.0 && hsv.hue <= 360.0)) {
		return 1;
	}
	if (!(hsv.sat >= 0.0 && hsv.sat <= 1.0)) {
		return 1;
	}
	if (!(hsv.val >= 0.0 && hsv.val <= 1.0)) {
		return 1;
	}
	return 0;
}

// https://dystopiancode.blogspot.com/2012/06/hsv-rgb-conversion-algorithms-in-c.html
// https://en.wikipedia.org/wiki/HSL_and_HSV
int rgb_d_from_hsv_d(struct rgb_d *rgb, struct hsv_d hsv)
{
	assert(rgb);
	assert(!invalid_hsv(hsv));

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
