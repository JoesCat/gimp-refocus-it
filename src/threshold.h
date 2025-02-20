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

#ifndef _THRESHOLD_H
#define _THRESHOLD_H

#include "compiler.h"
#include "convmask.h"
#include "image.h"

C_DECL_BEGIN

typedef struct {
  int x;
  int y;
  double *data;
} threshold_t;

threshold_t* threshold_create_mirror(threshold_t* threshold, convmask_t* convmask, image_t* image);
threshold_t* threshold_create_period(threshold_t* threshold, convmask_t* convmask, image_t* image);
void threshold_destroy(threshold_t* threshold);
double threshold_get(threshold_t* threshold, int x, int y);

C_DECL_END

#endif
