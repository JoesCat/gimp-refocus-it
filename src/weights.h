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

#ifndef _WEIGHTS_H
#define _WEIGHTS_H

#include "compiler.h"
#include "convmask.h"

C_DECL_BEGIN

typedef struct {
  double *w;
  int     r2;
  int     rxnz, rynz;
  int     stride;
  int     size;
} weights_t;

weights_t* weights_create(weights_t* weights, convmask_t* convmask);
void weights_destroy(weights_t* weights);
double weights_get(weights_t* weights, int x, int y);

#if defined(NDEBUG)
void weights_print(weights_t* weights, char* str);
#endif

C_DECL_END

#endif
