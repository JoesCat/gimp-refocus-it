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

#include "lambda.h"

static void get_variance_mirror(image_t* variance, image_t* img, double* pmin, double* pmax, int winsize) {
  int i, j, k, l;
  double sum, sum2;
  double c;
  double num_points;
  double var, minvar, maxvar;

  minvar = 1e20;
  maxvar = 0.0;
  num_points = (double)((2*winsize+1)*(2*winsize+1));

  for (i = 0; i < variance->x; i++) {
    for (j = 0; j < variance->y; j++) {
      sum = 0.0;
      sum2 = 0.0;
      for (k = -winsize; k <= winsize; k++) {
        for (l = -winsize; l <= winsize; l++) {
          c = image_get_mirror(img, i+k, j+l);
          sum += c;
          sum2 += c*c;
        }
      }
      sum /= num_points;
      sum2 /= num_points;
      sum *= sum;
      var = sum2 - sum;
      image_set(variance, i, j, var);
      if (var > maxvar) maxvar = var;
      if (var < minvar) minvar = var;
    }
  }
  *pmax = maxvar;
  *pmin = minvar;

#if defined(NDEBUG)
  printf("variance %d %d\n", variance->x, variance->y);
  for (i = 0; i < 5; i++) {
    for (j = 0; j < 5; j++) {
      printf("[%d %d %g", i, j, image_get (variance, i, j));
    }
    printf("\n");
  }
#endif
}

static void get_variance_period(image_t* variance, image_t* img, double* pmin, double* pmax, int winsize) {
  int i, j, k, l;
  double sum, sum2;
  double c;
  double num_points;
  double var, minvar, maxvar;

  minvar = 1e20;
  maxvar = 0.0;
  num_points = (double)((2*winsize+1)*(2*winsize+1));

  for (i = 0; i < variance->x; i++) {
    for (j = 0; j < variance->y; j++) {
      sum = 0.0;
      sum2 = 0.0;
      for (k = -winsize; k <= winsize; k++) {
        for (l = -winsize; l <= winsize; l++) {
          c = image_get_period(img, i+k, j+l);
          sum += c;
          sum2 += c*c;
        }
      }
      sum /= num_points;
      sum2 /= num_points;
      sum *= sum;
      var = sum2 - sum;
      image_set(variance, i, j, var);
      if (var > maxvar) maxvar = var;
      if (var < minvar) minvar = var;
    }
  }
  *pmax = maxvar;
  *pmin = minvar;

#if defined(NDEBUG)
  printf("variance %d %d\n", variance->x, variance->y);
  for (i = 0; i < 5; i++) {
    for (j = 0; j < 5; j++) {
      printf("[%d %d %g", i, j, image_get (variance, i, j));
    }
    printf("\n");
  }
#endif
}

lambda_t* lambda_create(lambda_t* lambda, int x, int y, double minlambda, int winsize, convmask_t* filter) {
  lambda->x = x;
  lambda->y = y;
  lambda->minlambda = minlambda;
  lambda->winsize = winsize;
  lambda->filter = filter;
  if ((lambda->lambda = (double*)calloc(x * y, sizeof(double))))
    return lambda;
#if defined(NDEBUG)
    printf("Error, lambda_create() - Out of memory!\n");
#endif
  return NULL;
}

void lambda_destroy(lambda_t* lambda) {
  free(lambda->lambda);
}

void lambda_set_mirror(lambda_t* lambda, int mirror) {
  lambda->mirror = mirror;
}

void lambda_set_nl(lambda_t* lambda, int nl) {
  lambda->nl = nl;
}

static lambda_t* lambda_calculate_period(lambda_t* lambda, image_t* image) {
  image_t imgenh, *imgcal;
  image_t variance;
  double minvar, maxvar;
  double akoef, bkoef;
  int i, size;

  if (lambda->filter) {
    if (!(imgcal = image_create_copyparam(&imgenh, image)))
      return NULL;
    if (!(image_convolve_period(imgcal, image, lambda->filter))) {
      image_destroy(imgcal);
      return NULL;
    }
  } else {
    imgcal = image;
  }

  if (!(image_create_copyparam(&variance, imgcal))) {
    if (imgcal == &imgenh)
      image_destroy(imgcal);
    return NULL;
  }

  get_variance_period(&variance, imgcal, &minvar, &maxvar, lambda->winsize);

  bkoef = (1.0 - lambda->minlambda)/(maxvar - minvar);
  akoef = 1.0 - (minvar*(1.0 - lambda->minlambda))/(maxvar - minvar);

  size = lambda->x * lambda->y;
  for (i = 0; i < size; i++) {
    lambda->lambda[i] = akoef + bkoef * variance.data[i];
  }

#if defined(NDEBUG)
  printf("lambda_calculate_period(), minvar=%g maxvar=%g bkoef=%g akoef=%g size=%d\n",
         minvar, maxvar, bkoef, akoef, size);
#endif

  if (lambda->filter) {
    image_destroy(imgcal);
  }
  image_destroy(&variance);

  return lambda;
}

static lambda_t* lambda_calculate_period_nl(lambda_t* lambda, image_t* image) {
  image_t imgenh, *imgcal;
  image_t variance;
  double minvar, maxvar;
  double alpha;
  int i, size;

  if (lambda->filter) {
    if (!(imgcal = image_create_copyparam(&imgenh, image)))
      return NULL;
    if (!(image_convolve_period(imgcal, image, lambda->filter))) {
      image_destroy(imgcal);
      return NULL;
    }
  } else {
    imgcal = image;
  }

  if (!(image_create_copyparam(&variance, imgcal))) {
    if (imgcal == &imgenh)
      image_destroy(imgcal);
    return NULL;
  }

  get_variance_period(&variance, imgcal, &minvar, &maxvar, lambda->winsize);

  alpha = (1.0 - lambda->minlambda)/(lambda->minlambda * (maxvar - minvar));

  size = lambda->x * lambda->y;
  for (i = 0; i < size; i++) {
    lambda->lambda[i] = 1.0/(1.0 + alpha * (variance.data[i] - minvar));
  }

#if defined(NDEBUG)
  printf("lambda_calculate_period_nl(), minvar=%g maxvar=%g alpha=%g size=%d\n",
         minvar, maxvar, alpha, size);
#endif

  if (lambda->filter) {
    image_destroy(imgcal);
  }
  image_destroy(&variance);

  return lambda;
}

static lambda_t* lambda_calculate_mirror(lambda_t* lambda, image_t* image) {
  image_t imgenh, *imgcal;
  image_t variance;
  double minvar, maxvar;
  double akoef, bkoef;
  int i, size;

  if (lambda->filter) {
    if (!(imgcal = image_create_copyparam(&imgenh, image)))
      return NULL;
    if (!(image_convolve_mirror(imgcal, image, lambda->filter))) {
      image_destroy(imgcal);
      return NULL;
    }
  } else {
    imgcal = image;
  }

  if (!(image_create_copyparam(&variance, imgcal))) {
    if (imgcal == &imgenh)
      image_destroy(imgcal);
    return NULL;
  }

  get_variance_mirror(&variance, imgcal, &minvar, &maxvar, lambda->winsize);

  bkoef = (1.0 - lambda->minlambda)/(maxvar - minvar);
  akoef = 1.0 - (minvar*(1.0 - lambda->minlambda))/(maxvar - minvar);

  size = lambda->x * lambda->y;
  for (i = 0; i < size; i++) {
    lambda->lambda[i] = akoef + bkoef * variance.data[i];
  }

#if defined(NDEBUG)
  printf("lambda_calculate_mirror(), minvar=%g maxvar=%g bkoef=%g akoef=%g size=%d\n",
         minvar, maxvar, bkoef, akoef, size);
#endif

  if (lambda->filter) {
    image_destroy(imgcal);
  }
  image_destroy(&variance);

  return lambda;
}

static lambda_t* lambda_calculate_mirror_nl(lambda_t* lambda, image_t* image) {
  image_t imgenh, *imgcal;
  image_t variance;
  double minvar, maxvar;
  double alpha;
  int i, size;

  if (lambda->filter) {
    if (!(imgcal = image_create_copyparam(&imgenh, image)))
      return NULL;
    if (!(image_convolve_mirror(imgcal, image, lambda->filter))) {
      image_destroy(imgcal);
      return NULL;
    }
  } else {
    imgcal = image;
  }

  if (!(image_create_copyparam(&variance, imgcal))) {
    if (imgcal == &imgenh)
      image_destroy(imgcal);
    return NULL;
  }

  get_variance_mirror(&variance, imgcal, &minvar, &maxvar, lambda->winsize);

  alpha = (1.0 - lambda->minlambda)/(lambda->minlambda*(maxvar - minvar));

  size = lambda->x * lambda->y;
  for (i = 0; i < size; i++) {
    lambda->lambda[i] = 1.0/(1.0 + alpha * (variance.data[i] - minvar));
  }

#if defined(NDEBUG)
  printf("lambda_calculate_mirror_nl(), minvar=%g maxvar=%g alpha=%g size=%d\n",
         minvar, maxvar, alpha, size);
#endif

  if (lambda->filter) {
    image_destroy(imgcal);
  }
  image_destroy(&variance);

  return lambda;
}

lambda_t* lambda_calculate(lambda_t* lambda, image_t* image) {
  if (lambda->mirror) {
    if (lambda->nl) return lambda_calculate_mirror_nl(lambda, image);
    else return lambda_calculate_mirror(lambda, image);
  } else {
    if (lambda->nl) return lambda_calculate_period_nl(lambda, image);
    else return lambda_calculate_period(lambda, image);
  }
}

double lambda_get_mirror(lambda_t* lambda, int x, int y) {
 return lambda->lambda[boundary_normalize_mirror(y, lambda->y)*lambda->x + boundary_normalize_mirror(x, lambda->x)];
}

double lambda_get_period(lambda_t* lambda, int x, int y) {
  return lambda->lambda[boundary_normalize_period(y, lambda->y)*lambda->x + boundary_normalize_period(x, lambda->x)];
}

