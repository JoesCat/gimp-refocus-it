/*
 * Written 2003 Lukas Kunc <Lukas.Kunc@seznam.cz>
 * Upgrade edits done 2025 by Joe Da Silva <digital@joescat.com>
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

#include "hopfield.h"

#define hardlim(x) ((x)>=0.0?1.0:-1.0)
#ifndef min
#define min(x,y) (((x) >= (y))?(y):(x))
#endif
#ifndef max
#define max(x,y) (((x) >= (y))?(x):(y))
#endif

/* Private functions */

static double hopfield_iteration_period(hopfield_t* hopfield) {
  double pom;
  int p, r;
  int i,j;
  double s;
  int k;
  double dE;
  double Sum;
  double ddui;
  double dk;
  double z;
  int x, y;
  double value;
  int rxnz, rynz;

  x = hopfield->image->x;
  y = hopfield->image->y;
  rxnz = hopfield->weights.rxnz;
  rynz = hopfield->weights.rynz;

  pom = weights_get(&(hopfield->weights), 0, 0) - 20.0 * hopfield->lambda;
  Sum = 0.0;
  for (i = 0; i < x; i++) {
    for (j = 0; j < y; j++) {
      s = 0.0;
      for (p = -rxnz; p <= rxnz; p++) {
        for (r = -rynz; r <= rynz; r++) {
          s += weights_get(&(hopfield->weights), p, r) * image_get_period(hopfield->image, i+p, j+r);
        }
      }

      z = 20.0 * image_get_period(hopfield->image, i,j);
      z += image_get_period(hopfield->image, i+2, j);
      z += image_get_period(hopfield->image, i-2, j);
      z += 2.0 * (image_get_period(hopfield->image, i+1, j-1) +
                  image_get_period(hopfield->image, i-1, j+1) +
                  image_get_period(hopfield->image, i+1, j+1) +
                  image_get_period(hopfield->image, i-1, j-1));
      z += image_get_period(hopfield->image, i, j+2);
      z += image_get_period(hopfield->image, i, j-2);
      z += -8.0 * (image_get_period(hopfield->image, i+1, j  ) +
                   image_get_period(hopfield->image, i  , j+1) +
                   image_get_period(hopfield->image, i-1, j  ) +
                   image_get_period(hopfield->image, i  , j-1));

      s -= hopfield->lambda*z;

      s += threshold_get(&(hopfield->threshold), i, j);

      ddui = hardlim(s);
      dE = -2.0 * s * ddui - pom;
      if (dE < 0.0) {
        value = image_get(hopfield->image, i, j);
        dk = ddui - (s/pom);
        if (dk > 0.5/2147483647.0 && value < 2147483646.5/2147483647.0) {
          k = (int)(2147483647.0 * min(dk, 1.0 - value) + 0.5/2147483647.0);
          dk = (1.0/2147483647.0 * (double)(rand()%k));
          value += dk;
          dE = (-2.0*s - pom*dk)*dk;
          Sum += dE;
          image_set(hopfield->image, i, j, value);
        } else if (dk < -0.5/2147483647.0 && value > 0.5/2147483647.0) {
          k = (int)(2147483647.0 * min(-dk, value) + 0.5/2147483647.0);
          dk = -(1.0/2147483647.0 * (double)(rand()%k));
          value += dk;
          dE = (-2.0*s - pom*dk)*dk;
          Sum += dE;
          image_set(hopfield->image, i, j, value);
        }
      }
    }
  }
  return Sum;
}

static double hopfield_iteration_period_lambda(hopfield_t* hopfield) {
  int p, r;
  int i, j;
  int k;
  double dE;
  double Sum;
  double ddui;
  double dk;
  double z;
  double value;
  double pom;
  double lmbd00, lmbd01, lmbd10, lmbd_10, lmbd0_1;
  double s;
  int x, y;
  int rxnz, rynz;

  x = hopfield->image->x;
  y = hopfield->image->y;
  rxnz = hopfield->weights.rxnz;
  rynz = hopfield->weights.rynz;

  Sum = 0.0;
  for (i = 0; i < x; i++) {
    for (j = 0; j < y; j++) {
      s = 0.0;
      for (p = -rxnz; p <= rxnz; p++) {
        for (r = -rynz; r <= rynz; r++) {
          s += weights_get(&(hopfield->weights), p, r) * image_get_period(hopfield->image, i+p, j+r);
        }
      }

      lmbd00  = lambda_get_period(hopfield->lambdafld, i  , j  );
      lmbd01  = lambda_get_period(hopfield->lambdafld, i  , j+1);
      lmbd10  = lambda_get_period(hopfield->lambdafld, i+1, j  );
      lmbd_10 = lambda_get_period(hopfield->lambdafld, i-1, j  );
      lmbd0_1 = lambda_get_period(hopfield->lambdafld, i  , j-1);

      pom = (lmbd01 + lmbd10 + lmbd_10 + lmbd0_1 + 16.0 * lmbd00);
      z = pom * image_get_period(hopfield->image, i, j);
      z += lmbd10 * image_get_period(hopfield->image, i+2, j);
      z += lmbd_10 * image_get_period(hopfield->image, i-2, j);
      z += (lmbd10 + lmbd0_1) * image_get_period(hopfield->image, i+1, j-1);
      z += (lmbd01 + lmbd_10) * image_get_period(hopfield->image, i-1, j+1);
      z += (lmbd10 + lmbd01) * image_get_period(hopfield->image, i+1, j+1);
      z += (lmbd0_1 + lmbd_10) * image_get_period(hopfield->image, i-1, j-1);
      z += -4.0 * (lmbd10 + lmbd00) * image_get_period(hopfield->image, i+1, j);
      z += -4.0 * (lmbd00 + lmbd_10) * image_get_period(hopfield->image, i-1, j);
      z += lmbd01 * image_get_period(hopfield->image, i, j+2);
      z += lmbd0_1 * image_get_period(hopfield->image, i, j-2);
      z += -4.0 * (lmbd01 + lmbd00) * image_get_period(hopfield->image, i, j+1);
      z += -4.0 * (lmbd00 + lmbd0_1) * image_get_period(hopfield->image, i, j-1);

      s -= hopfield->lambda*z;
      pom = -pom;
      pom *= hopfield->lambda;

      s += threshold_get(&(hopfield->threshold), i, j);
      pom += weights_get(&(hopfield->weights), 0, 0);

      ddui = hardlim(s);
      dE = -2.0 * s * ddui - pom;
      if (dE < 0.0) {
        value = image_get(hopfield->image, i, j);
        dk = ddui - (s/pom);
        if (dk > 0.5/2147483647.0 && value < 2147483646.5/2147483647.0) {
          k = (int)(2147483647.0 * min(dk, 1.0 - value) + 0.5/2147483647.0);
          dk = (1.0/2147483647.0 * (double)(rand()%k));
          value += dk;
          dE = (-2.0*s - pom*dk)*dk;
          Sum += dE;
          image_set(hopfield->image, i, j, value);
        } else if (dk < -0.5/2147483647.0 && value > 0.5/2147483647.0) {
          k = (int)(2147483647.0 * min(-dk, value) + 0.5/2147483647.0);
          dk = -(1.0/2147483647.0 * (double)(rand()%k));
          value += dk;
          dE = (-2.0*s - pom*dk)*dk;
          Sum += dE;
          image_set(hopfield->image, i, j, value);
        }
      }
    }
  }
  return Sum;
}

static double hopfield_iteration_mirror(hopfield_t* hopfield) {
  double pom;
  int p, r;
  int i,j;
  double s;
  int k;
  double dE;
  double Sum;
  double ddui;
  double dk;
  double z;
  int x, y;
  double value;
  int rxnz, rynz;

  x = hopfield->image->x;
  y = hopfield->image->y;
  rxnz = hopfield->weights.rxnz;
  rynz = hopfield->weights.rynz;

  pom = weights_get(&(hopfield->weights), 0, 0) - 20.0 * hopfield->lambda;
  Sum = 0.0;
  for (i = 0; i < x; i++) {
    for (j = 0; j < y; j++) {
      s = 0.0;
      for (p = -rxnz; p <= rxnz; p++) {
        for (r = -rynz; r <= rynz; r++) {
          s += weights_get(&(hopfield->weights), p, r) * image_get_mirror(hopfield->image, i+p, j+r);
        }
      }

      z = 20.0 * image_get_mirror(hopfield->image, i,j);
      z += image_get_mirror(hopfield->image, i+2, j);
      z += image_get_mirror(hopfield->image, i-2, j);
      z += 2.0 * (image_get_mirror(hopfield->image, i+1, j-1) +
                  image_get_mirror(hopfield->image, i-1, j+1) +
                  image_get_mirror(hopfield->image, i+1, j+1) +
                  image_get_mirror(hopfield->image, i-1, j-1));
      z += image_get_mirror(hopfield->image, i, j+2);
      z += image_get_mirror(hopfield->image, i, j-2);
      z += -8.0 * (image_get_mirror(hopfield->image, i+1, j  ) +
                   image_get_mirror(hopfield->image, i  , j+1) +
                   image_get_mirror(hopfield->image, i-1, j  ) +
                   image_get_mirror(hopfield->image, i  , j-1));

      s -= hopfield->lambda*z;

      s += threshold_get(&(hopfield->threshold), i, j);

      ddui = hardlim(s);
      dE = -2.0 * s * ddui - pom;
      if (dE < 0.0) {
        value = image_get(hopfield->image, i, j);
        dk = ddui - (s/pom);
        if (dk > 0.5/2147483647.0 && value < 2147483646.5/2147483647.0) {
          k = (int)(2147483647.0 * min(dk, 1.0 - value) + 0.5/2147483647.0);
          dk = (1.0/2147483647.0 * (double)(rand()%k));
          value += dk;
          dE = (-2.0*s - pom*dk)*dk;
          Sum += dE;
          image_set(hopfield->image, i, j, value);
        } else if (dk < -0.5/2147483647.0 && value > 0.5/2147483647.0) {
          k = (int)(2147483647.0 * min(-dk, value) + 0.5/2147483647.0);
          dk = -(1.0/2147483647.0 * (double)(rand()%k));
          value += dk;
          dE = (-2.0*s - pom*dk)*dk;
          Sum += dE;
          image_set(hopfield->image, i, j, value);
        }
      }
    }
  }
  return Sum;
}

static double hopfield_iteration_mirror_lambda(hopfield_t* hopfield) {
  int p, r;
  int i, j;
  int k;
  double dE;
  double Sum;
  double ddui;
  double dk;
  double z;
  double value;
  double pom;
  double lmbd00, lmbd01, lmbd10, lmbd_10, lmbd0_1;
  double s;
  int x, y;
  int rxnz, rynz;

  x = hopfield->image->x;
  y = hopfield->image->y;
  rxnz = hopfield->weights.rxnz;
  rynz = hopfield->weights.rynz;

  Sum = 0.0;
  for (i = 0; i < x; i++) {
    for (j = 0; j < y; j++) {
      s = 0.0;
      for (p = -rxnz; p <= rxnz; p++) {
        for (r = -rynz; r <= rynz; r++) {
          s += weights_get(&(hopfield->weights), p, r) * image_get_mirror(hopfield->image, i+p, j+r);
        }
      }

      lmbd00  = lambda_get_mirror(hopfield->lambdafld, i  , j  );
      lmbd01  = lambda_get_mirror(hopfield->lambdafld, i  , j+1);
      lmbd10  = lambda_get_mirror(hopfield->lambdafld, i+1, j  );
      lmbd_10 = lambda_get_mirror(hopfield->lambdafld, i-1, j  );
      lmbd0_1 = lambda_get_mirror(hopfield->lambdafld, i  , j-1);

      pom = (lmbd01 + lmbd10 + lmbd_10 + lmbd0_1 + 16.0 * lmbd00);
      z = pom * image_get_mirror(hopfield->image, i, j);
      z += lmbd10 * image_get_mirror(hopfield->image, i+2, j);
      z += lmbd_10 * image_get_mirror(hopfield->image, i-2, j);
      z += (lmbd10 + lmbd0_1) * image_get_mirror(hopfield->image, i+1, j-1);
      z += (lmbd01 + lmbd_10) * image_get_mirror(hopfield->image, i-1, j+1);
      z += (lmbd10 + lmbd01) * image_get_mirror(hopfield->image, i+1, j+1);
      z += (lmbd0_1 + lmbd_10) * image_get_mirror(hopfield->image, i-1, j-1);
      z += -4.0 * (lmbd10 + lmbd00) * image_get_mirror(hopfield->image, i+1, j);
      z += -4.0 * (lmbd00 + lmbd_10) * image_get_mirror(hopfield->image, i-1, j);
      z += lmbd01 * image_get_mirror(hopfield->image, i, j+2);
      z += lmbd0_1 * image_get_mirror(hopfield->image, i, j-2);
      z += -4.0 * (lmbd01 + lmbd00) * image_get_mirror(hopfield->image, i, j+1);
      z += -4.0 * (lmbd00 + lmbd0_1) * image_get_mirror(hopfield->image, i, j-1);

      s -= hopfield->lambda*z;
      pom = -pom;
      pom *= hopfield->lambda;

      s += threshold_get(&(hopfield->threshold), i, j);
      pom += weights_get(&(hopfield->weights), 0, 0);

      ddui = hardlim(s);
      dE = -2.0 * s * ddui - pom;
      if (dE < 0.0) {
        value = image_get(hopfield->image, i, j);
        dk = ddui - (s/pom);
        if (dk > 0.5/2147483647.0 && value < 2147483646.5/2147483647.0) {
          k = (int)(2147483647.0 * min(dk, 1.0 - value) + 0.5/2147483647.0);
          dk = (1.0/2147483647.0 * (double)(rand()%k));
          value += dk;
          dE = (-2.0*s - pom*dk)*dk;
          Sum += dE;
          image_set(hopfield->image, i, j, value);
        } else if (dk < -0.5/2147483647.0 && value > 0.5/2147483647.0) {
          k = (int)(2147483647.0 * min(-dk, value) + 0.5/2147483647.0);
          dk = -(1.0/2147483647.0 * (double)(rand()%k));
          value += dk;
          dE = (-2.0*s - pom*dk)*dk;
          Sum += dE;
          image_set(hopfield->image, i, j, value);
        }
      }
    }
  }
  return Sum;
}

static hopfield_t* hopfield_create_mirror(hopfield_t* hopfield, convmask_t* convmask, image_t* image, lambda_t* lambdafld) {
  hopfield->image = image;
  hopfield->mirror = 1;
  if (!(weights_create(&(hopfield->weights), convmask)))
    return NULL;
  if (!(threshold_create_mirror(&(hopfield->threshold), convmask, image))) {
    weights_destroy(&(hopfield->weights));
    return NULL;
  }
  hopfield->lambdafld = lambdafld;
  return hopfield;
}

static hopfield_t* hopfield_create_period(hopfield_t* hopfield, convmask_t* convmask, image_t* image, lambda_t* lambdafld) {
  hopfield->image = image;
  hopfield->mirror = 0;
  if (!(weights_create(&(hopfield->weights), convmask)))
    return NULL;
  if (!(threshold_create_mirror(&(hopfield->threshold), convmask, image))) {
    weights_destroy(&(hopfield->weights));
    return NULL;
  }
  hopfield->lambdafld = lambdafld;
  return hopfield;
}

/* Public functions */

hopfield_t* hopfield_create(hopfield_t* hopfield, convmask_t* convmask, image_t* image, lambda_t* lambdafld) {
  if (hopfield->mirror) return hopfield_create_mirror(hopfield, convmask, image, lambdafld);
  else return hopfield_create_period(hopfield, convmask, image, lambdafld);
}

void hopfield_destroy(hopfield_t* hopfield) {
  weights_destroy(&(hopfield->weights));
  threshold_destroy(&(hopfield->threshold));
}

double hopfield_iteration(hopfield_t* hopfield) {
  double rv;
  if (hopfield->mirror) {
    if (hopfield->lambdafld && hopfield->lambda > 1e-8) rv = hopfield_iteration_mirror_lambda(hopfield);
    else rv = hopfield_iteration_mirror(hopfield);
  } else {
    if (hopfield->lambdafld && hopfield->lambda > 1e-8) rv = hopfield_iteration_period_lambda(hopfield);
    else rv = hopfield_iteration_period(hopfield);
  }
  return rv;
}

void hopfield_set_mirror(hopfield_t* hopfield, int mirror) {
  hopfield->mirror = mirror;
}
