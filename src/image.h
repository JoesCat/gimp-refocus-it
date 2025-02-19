/*
 * Written 2003 Lukas Kunc <Lukas.Kunc@seznam.cz>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef _IMAGE_H
#define _IMAGE_H

#include <stdio.h>
#include "compiler.h"
#include "convmask.h"
#include "boundary.h"

C_DECL_BEGIN

typedef struct {
  int     x;
  int     y;
  real_t *data;
} image_t;

void image_init(image_t* image);
image_t* image_create(image_t* image, int x, int y);
image_t* image_create_copyparam(image_t* image, image_t* src);
void image_destroy(image_t* image);

int image_load_pnm_file(image_t* imageR, image_t* imageG, image_t* imageB, int* bpp, FILE* file);
int image_save_pnm_file(image_t* imageR, image_t* imageG, image_t* imageB, int binary, FILE* file);
int image_load_pnm(image_t* imageR, image_t* imageG, image_t* imageB, int* bpp, const char* name);
int image_save_pnm(image_t* imageR, image_t* imageG, image_t* imageB, int binary, const char* name);
void image_load_bytes_gray(image_t* image, unsigned char* bytes);
void image_load_bytes_rgb(image_t* image, unsigned char* bytes, unsigned int channel);

image_t* image_convolve_mirror(image_t* dst, image_t* src, convmask_t* filter);
image_t* image_convolve_period(image_t* dst, image_t* src, convmask_t* filter);

real_t image_get_mirror(image_t* image, int x, int y);
real_t image_get_period(image_t* image, int x, int y);
void image_set(image_t* image, int x, int y, real_t value);
real_t image_get(image_t* image, int x, int y);

C_DECL_END

#endif
