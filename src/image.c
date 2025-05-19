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

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "image.h"

image_t* image_create(image_t* image, int x, int y) {
  image->x = x;
  image->y = y;
  if ((image->data = (double*)malloc(sizeof(double) * x * y)))
    return image;
  return NULL;
}

image_t* image_create_copyparam(image_t* image, image_t* src) {
  image->x = src->x;
  image->y = src->y;
  if ((image->data = (double*)malloc(sizeof(double) * image->x * image->y)))
    return image;
  return NULL;
}

void image_destroy(image_t* image) {
  free(image->data);
}

/* not needed
 * static void image_init(image_t* image) {
 *   image->data = NULL;
 * }
 */

double image_get(image_t* image, int x, int y) {
  return (image->data[y * image->x + x]);
}

void image_set(image_t* image, int x, int y, double value) {
  image->data[y * image->x + x] = value;
}

image_t* image_convolve_mirror(image_t* dst, image_t* src, convmask_t* filter) {
  int i, j, k, l, r;
  double value;

  r = filter->radius;
  for (i = 0; i < src->x; i++) {
    for (j = 0; j < src->y; j++) {
      value = 0.0;
      for (k = -r; k <= r; k++) {
        for (l = -r; l <= r; l++) {
          value += convmask_get(filter, k,l) * image_get_mirror(src, i-k, j-l);
        }
      }
      image_set(dst, i, j, value);
    }
  }
  return dst;
}

image_t* image_convolve_period(image_t* dst, image_t* src, convmask_t* filter) {
  int i, j, k, l, r;
  double value;

  r = filter->radius;
  for (i = 0; i < src->x; i++) {
    for (j = 0; j < src->y; j++) {
      value = 0.0;
      for (k = -r; k <= r; k++) {
        for (l = -r; l <= r; l++) {
          value += convmask_get(filter, k,l) * image_get_period(src, i-k, j-l);
        }
      }
      image_set(dst, i, j, value);
    }
  }
  return dst;
}

double image_get_mirror(image_t* image, int x, int y) {
  return image->data[boundary_normalize_mirror(y, image->y) * image->x + boundary_normalize_mirror(x, image->x)];
}

double image_get_period(image_t* image, int x, int y) {
  return image->data[boundary_normalize_period(y, image->y) * image->x + boundary_normalize_period(x, image->x)];
}
