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

#ifndef _LAMBDA_H
#define _LAMBDA_H

#include "compiler.h"
#include "convmask.h"
#include "image.h"
#include "boundary.h"

C_DECL_BEGIN

typedef struct {
  convmask_t *filter;
  int         x;
  int         y;
  int         winsize;
  double      minlambda;
  double     *lambda;
  int         mirror;
  int         nl;
} lambda_t;

lambda_t* lambda_create(lambda_t* lambda, int x, int y, double minlambda, int winsize, convmask_t* filter);
void lambda_destroy(lambda_t* lambda);

lambda_t* lambda_calculate_period(lambda_t* lambda, image_t* image);
lambda_t* lambda_calculate_period_nl(lambda_t* lambda, image_t* image);
lambda_t* lambda_calculate_mirror(lambda_t* lambda, image_t* image);
lambda_t* lambda_calculate_mirror_nl(lambda_t* lambda, image_t* image);
lambda_t* lambda_calculate(lambda_t* lambda, image_t* image);

void lambda_set_mirror(lambda_t* lambda, int mirror);
void lambda_set_nl(lambda_t* lambda, int nl);

double lambda_get_mirror(lambda_t* lambda, int x, int y);
double lambda_get_period(lambda_t* lambda, int x, int y);

C_DECL_END

#endif
