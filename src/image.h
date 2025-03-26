/*
 * Written 2003 Lukas Kunc <Lukas.Kunc@seznam.cz>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef _IMAGE_H
#define _IMAGE_H

#include "compiler.h"
#include "convmask.h"
#include "boundary.h"

C_DECL_BEGIN

typedef struct {
  int     x;
  int     y;
  double *data;
} image_t;

image_t* image_create(image_t* image, int x, int y);
image_t* image_create_copyparam(image_t* image, image_t* src);
void image_destroy(image_t* image);
double image_get(image_t* image, int x, int y);
void image_set(image_t* image, int x, int y, double value);

image_t* image_convolve_mirror(image_t* dst, image_t* src, convmask_t* filter);
image_t* image_convolve_period(image_t* dst, image_t* src, convmask_t* filter);

double image_get_mirror(image_t* image, int x, int y);
double image_get_period(image_t* image, int x, int y);

C_DECL_END

#endif
