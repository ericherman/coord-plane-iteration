/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* rgb-hsv.h: colors */
/* Copyright (C) 2020 Eric Herman <eric@freesa.org> */

#ifndef RGB_HSV_H
#define RGB_HSV_H

#include <stdint.h>

typedef struct rgb24 {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
} rgb24_s;

// values are from 0.0 to 1.0
typedef struct rgb {
	double red;
	double green;
	double blue;
} rgb_s;

// hue is 0.0 to 360.0
// saturation and value are 0.0 to 0.1
typedef struct hsv {
	double hue;
	double sat;
	double val;
} hsv_s;

int rgb24_from_rgb(rgb24_s *out, rgb_s in);
int rgb24_from_uint32(rgb24_s *out, uint32_t in);

int rgb_from_hsv(rgb_s *rgb, hsv_s hsv);

uint32_t rgb24_to_uint32(rgb24_s rgb);

#endif /* RGB_HSV_H */
