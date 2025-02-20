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

#ifndef _HOPFIELD_H
#define _HOPFIELD_H

#include "compiler.h"
#include "convmask.h"
#include "image.h"
#include "weights.h"
#include "threshold.h"
#include "lambda.h"

C_DECL_BEGIN

typedef struct {
  int         mirror;
  image_t    *image;
  weights_t   weights;
  double      lambda;
  lambda_t   *lambdafld;
  threshold_t threshold;
} hopfield_t;

hopfield_t* hopfield_create_mirror(hopfield_t* hopfield, convmask_t* convmask, image_t* image, lambda_t* lambdafld);
hopfield_t* hopfield_create_period(hopfield_t* hopfield, convmask_t* convmask, image_t* image, lambda_t* lambdafld);
hopfield_t* hopfield_create(hopfield_t* hopfield, convmask_t* convmask, image_t* image, lambda_t* lambdafld);
void hopfield_set_mirror(hopfield_t* hopfield, int mirror);
void hopfield_destroy(hopfield_t* hopfield);
double hopfield_iteration(hopfield_t* hopfield);

C_DECL_END

#endif
