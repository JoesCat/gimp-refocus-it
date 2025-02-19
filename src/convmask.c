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
#include "convmask.h"

convmask_t* convmask_create(convmask_t* convmask, int radius)
{
  convmask->radius = radius;
  radius *= 2;
  radius += 1;
  convmask->r21 = radius;
  convmask->speeder = convmask->radius * (convmask->r21 + 1);
  if ((convmask->coef = malloc(sizeof(real_t) * radius * radius)))
    return convmask;
  /* out of memory, returm NULL */
  return NULL;
}


convmask_t* convmask_convolve(convmask_t* ct, convmask_t* c1, convmask_t* c2)
{
  int x, y, r, r2, x0, y0;
  real_t sum;
  convmask_t ctmp;

  if (!(convmask_create(ct, c1->radius + c2->radius)))
    return NULL;
  if (!(convmask_create(&ctmp, c1->radius + c2->radius)))
  {
    convmask_destroy(ct);
    return NULL;
  };

  r = ctmp.r21 * ctmp.r21;
  for (x = 0; x < r; x++)
  {
    ctmp.coef[x] = 0.0;
  }

  r = c1->radius;
  for (x = -r; x <= r; x++)
  {
    for (y = -r; y <= r; y++)
    {
      convmask_set(&ctmp, x, y, convmask_get(c1, x, y));
    }
  }

  r = ctmp.radius;
  r2 = c2->radius;
  for (x = -r; x <= r; x++)
  {
    for (y = -r; y <= r; y++)
    {
      sum = 0.0;
      for (x0 = -r2; x0 <= r2; x0++)
      {
        for (y0 = -r2; y0 <= r2; y0++)
        {
          sum += convmask_get_0(&ctmp, x - x0, y - y0) * convmask_get(c2, x0, y0);
        }
      }
      convmask_set(ct, x, y, sum);
    }
  }
  convmask_destroy(&ctmp);

  return ct;
}

void convmask_destroy(convmask_t* convmask)
{
  free(convmask->coef);
}

void convmask_set_circle(convmask_t* convmask, int i, int j, real_t value)
{
  convmask_set(convmask,  i,  j, value);
  convmask_set(convmask, -i,  j, value);
  convmask_set(convmask,  i, -j, value);
  convmask_set(convmask, -i, -j, value);
}

convmask_t* convmask_normalize(convmask_t* convmask)
{
  int size, i;
  real_t sum;

  size = convmask->r21;
  size *= size;
  sum = R(0.0);
  for (i = 0; i < size; i++) sum += convmask->coef[i];
  for (i = 0; i < size; i++) convmask->coef[i] /= sum;

  return convmask;
}

void convmask_print(convmask_t* convmask, FILE* file)
{
  int i, j;
  fprintf(file, "%s\n", "CONVMASK:");
  for (i = -convmask->radius; i <= convmask->radius; i++)
  {
    for (j = -convmask->radius; j <= convmask->radius; j++)
    {
      fprintf(file, "%1.4f ", (float)convmask_get(convmask, j, i));
    }
    fprintf(file, "\n");
  }
}

void convmask_set(convmask_t* convmask, int i, int j, real_t value)
{
  convmask->coef[j*convmask->r21 + convmask->speeder + i] = value;
}

real_t convmask_get(convmask_t* convmask, int i, int j)
{
  return (convmask->coef[j*convmask->r21 + convmask->speeder + i]);
}

real_t convmask_get_0(convmask_t* convmask, int i, int j)
{
  return ((abs(i) <= convmask->radius && abs(j) <= convmask->radius) ? convmask_get(convmask, i, j) : R(0.0));
}
