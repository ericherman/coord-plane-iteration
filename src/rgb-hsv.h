/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* rgb-hsv.h: colors */
/* Copyright (C) 2020-2026 Eric Herman <eric@freesa.org> */

#ifndef RGB_HSV_H
#define RGB_HSV_H

#include <stdint.h>

struct rgb_24 {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
};

// values are from 0.0 to 1.0
struct rgb_d {
	double red;
	double green;
	double blue;
};

// hue is 0.0 to 360.0
// saturation and value are 0.0 to 0.1
struct hsv_d {
	double hue;
	double sat;
	double val;
};

int rgb_d_from_hsv_d(struct rgb_d *out, struct hsv_d in);

int rgb_24_from_rgb_d(struct rgb_24 *out, struct rgb_d in);

int rgb_24_from_uint32(struct rgb_24 *out, uint32_t in);

uint32_t rgb_24_to_uint32(struct rgb_24 rgb);

#endif /* RGB_HSV_H */
