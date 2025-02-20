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

#ifndef _CONVMASK_H
#define _CONVMASK_H

#include "compiler.h"

C_DECL_BEGIN

/** structures */

typedef struct {
  int     radius;
  int     r21;
  int     speeder;
  double *coef;
} convmask_t;

convmask_t* convmask_create(convmask_t* convmask, int radius);
void convmask_destroy(convmask_t* convmask);
convmask_t* convmask_normalize(convmask_t* convmask);
convmask_t* convmask_convolve(convmask_t* ct, convmask_t* c1, convmask_t* c2);
void convmask_set(convmask_t* convmask, int i, int j, double value);
double convmask_get(convmask_t* convmask, int i, int j);
void convmask_set_circle(convmask_t* convmask, int i, int j, double value);

#if defined(NDEBUG)
#include <stdio.h>
#include <stdlib.h>
void convmask_print(convmask_t* convmask, FILE* file);
#endif

C_DECL_END

#endif
