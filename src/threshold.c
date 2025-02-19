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

#include <stdlib.h>
#include "threshold.h"

threshold_t* threshold_create_mirror(threshold_t* threshold, convmask_t* convmask, image_t* image)
{
  int i,j;
  int k,l;
  real_t s;
  int x, y, r;

  threshold->x = x = image->x;
  threshold->y = y = image->y;
  r = convmask->radius;
  if (!(threshold->data = (real_t*)malloc(sizeof(real_t) * x * y)))
    return NULL;
  for (i = 0; i < x; i++) {
    for (j = 0; j < y; j++) {
      s = R(0.0);
      for (k = -r; k <= r; k++) {
        for (l = -r; l <= r; l++) {
          s += convmask_get(convmask, k, l) * image_get_mirror(image, k + i, l + j);
        }
      }
      threshold->data[j * x + i] = s;
    }
  }
  return threshold;
}

threshold_t* threshold_create_period(threshold_t* threshold, convmask_t* convmask, image_t* image)
{
  int i,j;
  int k,l;
  real_t s;
  int x, y, r;

  threshold->x = x = image->x;
  threshold->y = y = image->y;
  r = convmask->radius;
  if (!(threshold->data = (real_t*)malloc(sizeof(real_t) * x * y)))
    return NULL;
  for (i = 0; i < x; i++) {
    for (j = 0; j < y; j++) {
      s = R(0.0);
      for (k = -r; k <= r; k++) { 
        for (l = -r; l <= r; l++) {
          s += convmask_get(convmask, k, l) * image_get_period(image, k + i, l + j);
        }
      }
      threshold->data[j * x + i] = s;
    }
  }
  return threshold;
}

void threshold_destroy(threshold_t* threshold)
{
  free(threshold->data);
}

real_t threshold_get(threshold_t* threshold, int x, int y)
{
  return threshold->data[y * threshold->x + x];
}
