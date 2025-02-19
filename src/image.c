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
#include <string.h>
#include <errno.h>
#include "image.h"

#define LINE_LEN_BORDER_PPM 56
#define LINE_LEN_BORDER_PGM 64

typedef enum {
  PBM_ASCII  = 1,
  PGM_ASCII  = 2,
  PPM_ASCII  = 3,
  PBM_BINARY = 4,
  PGM_BINARY = 5,
  PPM_BINARY = 6,
  PNM_UNKNOWN
} PNMType;

image_t* image_create(image_t* image, int x, int y)
{
  image->x = x;
  image->y = y;
  if ((image->data = (real_t*)malloc(sizeof(real_t) * x * y)))
    return image;
  return NULL;
}

image_t* image_create_copyparam(image_t* image, image_t* src)
{
  image->x = src->x;
  image->y = src->y;
  if ((image->data = (real_t*)malloc(sizeof(real_t) * image->x * image->y)))
    return image;
  return NULL;
}

void image_init(image_t* image)
{
  image->data = NULL;
}

void image_destroy(image_t* image)
{
  free(image->data);
}

image_t* image_convolve_mirror(image_t* dst, image_t* src, convmask_t* filter)
{
  int i, j, k, l, r;
  real_t value;

  r = filter->radius;
  for (i = 0; i < src->x; i++)
  {
    for (j = 0; j < src->y; j++)
    {
      value = R(0.0);
      for (k = -r; k <= r; k++)
      {
	for (l = -r; l <= r; l++)
	{
	  value += convmask_get(filter, k,l) * image_get_mirror(src, i-k, j-l);
	}
      }
      image_set(dst, i, j, value);
    }
  }
  return dst;
}
	  
image_t* image_convolve_period(image_t* dst, image_t* src, convmask_t* filter)
{
  int i, j, k, l, r;
  real_t value;

  r = filter->radius;
  for (i = 0; i < src->x; i++)
  {
    for (j = 0; j < src->y; j++)
    {
      value = R(0.0);
      for (k = -r; k <= r; k++)
      {
	for (l = -r; l <= r; l++)
	{
	  value += convmask_get(filter, k,l) * image_get_period(src, i-k, j-l);
	}
      }
      image_set(dst, i, j, value);
    }
  }
  return dst;
}

int image_load_pnm_file(image_t* imageR, image_t* imageG, image_t* imageB, int* bpp, FILE* file)
{
  char buff[2];
  unsigned char bytesRGB[3];
  int c;
  int i, x, y, Y, valueR, valueG, valueB;
  unsigned char byte;
  PNMType type;
  real_t scale;

  if (imageR) image_init(imageR);
  if (imageG) image_init(imageG);
  if (imageB) image_init(imageB);

  if (fread(buff, sizeof(buff), 1, file) != 1)
  {
    return -1;
  }

  if (buff[0] != 'P')
  {
    errno = EINVAL;
    return -1;
  }

  switch (buff[1])
  {
    case '1':
      /* PBM, ascii */
      type = PBM_ASCII;
      break;
    case '2':
      /* PGM, ascii */
      type = PGM_ASCII;
      break;
    case '3':
      /* PPM, ascii */
      type = PPM_ASCII;
      break;
    case '4':
      /* PBM, binary */
      type = PBM_BINARY;
      break;
    case '5':
      /* PGM binary */
      type = PGM_BINARY;
      break;
    case '6':
      /* PPM, binary */
      type = PPM_BINARY;
      break;
    default:
      /* Unsupported */
      type = PNM_UNKNOWN;
      break;
  }
  if (type == PNM_UNKNOWN)
  {
    errno = EINVAL;
    return -1;
  }

  do
  {
    c = getc(file);
  }
  while (c == '\t' || c == ' ');
  if (c == '\r') c = getc(file);
  if (c != '\n')
  {
    errno = EINVAL;
    return -1;
  }
  c = getc(file);
  while (c == '#')
  {
    while (((c = getc(file)) != '\n') && (c != EOF));
    if (c != EOF) c = getc(file);
  }

  if (c == EOF)
  {
    errno = EINVAL;
    return -1;
  }
  ungetc(c, file);

  if (fscanf(file, "%d %d %d", &x, &y, &c) != 3)
  {
    errno = EINVAL;
    return -1;
  }
  scale = R(255.0) / (real_t)c;
  c = getc(file);
  if (c == '\r') c = getc(file);
  if (c != '\n')
  {
    errno = EINVAL;
    return -1;
  }

  Y = y;

  switch (type)
  {
    case PGM_ASCII:
      if (bpp) *bpp = 1;
      image_create(imageR, x, y);
      y = 0; i = 0;
      while (y < Y && fscanf(file, "%d", &valueR) == 1) {
        image_set(imageR, i, y, scale * (real_t)valueR);
        i++;
        if (i >= x) {
          i = 0;
          y++;
        }
      }
      break;
    case PGM_BINARY:
      if (bpp) *bpp = 1;
      image_create(imageR, x, y);
      y = 0; i = 0;
      while (y < Y && fread(&byte, sizeof(byte), 1, file) == 1) {
        image_set(imageR, i, y, scale * (real_t)byte);
        i++;
        if (i >= x) {
          i = 0;
          y++;
        }
      }
      break;
    case PPM_ASCII:
      if (bpp) *bpp = 3;
      image_create(imageR, x, y);
      if (!imageG || !imageR)
      {
        errno = EINVAL;
        return -1;
      }
      image_create(imageG, x, y);
      image_create(imageB, x, y);
      y = 0; i = 0;
      while (y < Y && fscanf(file, "%d %d %d", &valueR, &valueG, &valueB) == 3) {
        image_set(imageR, i, y, scale * (real_t)valueR);
        image_set(imageG, i, y, scale * (real_t)valueG);
        image_set(imageB, i, y, scale * (real_t)valueB);
        i++;
        if (i >= x) {
          i = 0;
          y++;
        }
      }
      break;
    case PPM_BINARY:
      if (bpp) *bpp = 3;
      image_create(imageR, x, y);
      if (!imageG || !imageR)
      {
        errno = EINVAL;
        return -1;
      }
      image_create(imageG, x, y);
      image_create(imageB, x, y);
      y = 0; i = 0;
      while (y < Y && fread(&bytesRGB, sizeof(bytesRGB), 1, file) == 1) {
        image_set(imageR, i, y, scale * (real_t)bytesRGB[0]);
        image_set(imageG, i, y, scale * (real_t)bytesRGB[1]);
        image_set(imageB, i, y, scale * (real_t)bytesRGB[2]);
        i++;
        if (i >= x) {
          i = 0;
          y++;
        }
      }
      break;
    default:
      errno = EINVAL;
      return -1;
  }

  return 0;
}


int image_save_pnm_file(image_t* imageR, image_t* imageG, image_t* imageB, int binary, FILE* file)
{
  int type;
  int i, j, rv, R, G, B;
  unsigned char c;
  int chars_cnt;

  if (imageG && imageG->data && imageB && imageB->data) type = 3;
  else type = 2;
  if (binary) type+=3;

  if (fprintf(file, "P%d\n# Deblur output\n%d %d\n255", type, imageR->x, imageR->y) < 0) return -1;

  switch (type)
  {
    case PGM_ASCII:
      chars_cnt = LINE_LEN_BORDER_PGM + 1;
      for (j = 0; j < imageR->y; j++)
      {
        for (i = 0; i < imageR->x; i++)
        {
          R = (int)(image_get(imageR, i, j) + R(0.5));
          if (chars_cnt > LINE_LEN_BORDER_PGM)
          {
            if (fprintf(file, "\n") < 0) return -1;
            chars_cnt = 0;
            rv = fprintf(file, "%d", R);
          }
          else
          {
            rv = fprintf(file, " %d", R);
          }
          if (rv < 0) return -1;
          chars_cnt += rv;
        }
      }
      break;
    case PGM_BINARY:
      if (fprintf(file, "\n") < 0) return -1;
      for (j = 0; j < imageR->y; j++)
      {
        for (i = 0; i < imageR->x; i++)
        {
          c = (unsigned char)(image_get(imageR, i, j) + R(0.5));
          if (fwrite(&c, sizeof(c), 1, file) != 1) return -1;
        }
      }
      break;
    case PPM_ASCII:
      chars_cnt = LINE_LEN_BORDER_PPM + 1;
      for (j = 0; j < imageR->y; j++)
      {
        for (i = 0; i < imageR->x; i++)
        {
          R = (int)(image_get(imageR, i, j) + R(0.5));
          G = (int)(image_get(imageG, i, j) + R(0.5));
          B = (int)(image_get(imageB, i, j) + R(0.5));
          if (chars_cnt > LINE_LEN_BORDER_PPM)
          {
            if (fprintf(file, "\n") < 0) return -1;
            chars_cnt = 0;
            rv = fprintf(file, "%d %d %d", R, G, B);
          }
          else
          {
            rv = fprintf(file, " %d %d %d", R, G, B);
          }
          if (rv < 0) return -1;
          chars_cnt += rv;
        }
      }
      break;
    case PPM_BINARY:
      if (fprintf(file, "\n") < 0) return -1;
      for (j = 0; j < imageR->y; j++)
      {
        for (i = 0; i < imageR->x; i++)
        {
          c = (unsigned char)(image_get(imageR, i, j) + R(0.5));
          if (fwrite(&c, sizeof(c), 1, file) != 1) return -1;
          c = (unsigned char)(image_get(imageG, i, j) + R(0.5));
          if (fwrite(&c, sizeof(c), 1, file) != 1) return -1;
          c = (unsigned char)(image_get(imageB, i, j) + R(0.5));
          if (fwrite(&c, sizeof(c), 1, file) != 1) return -1;
        }
      }
      break;
    default:
      errno = EINVAL;
      return -1;
  }

  return 0;
}

int image_save_pnm(image_t* imageR, image_t* imageG, image_t* imageB, int binary, const char* name)
{
  int retval;
  FILE *file;
  file = fopen(name, "wb");
  if (file) {
    retval = image_save_pnm_file(imageR, imageG, imageB, binary, file);
    fclose(file);
  }
  else {
	retval = -1;
  }
  return retval;
}

int image_load_pnm(image_t* imageR, image_t* imageG, image_t* imageB, int* bpp, const char* name)
{
  FILE *file;
  int retval;
  file = fopen(name, "rb");
  if (file) {
    retval = image_load_pnm_file(imageR, imageG, imageB, bpp, file);
    fclose(file);
  }
  else {
	retval = -1;
  }
  return retval;
}

void image_load_bytes_gray(image_t* image, unsigned char* src)
{
	int size, i;
	real_t *dst;
	size = image->x * image->y;

	dst = image->data;
	for (i = 0; i < size; i++)
	{
		*dst = *src;
		src++;
		dst++;
	}
}

void image_load_bytes_rgb(image_t* image, unsigned char* bytes, unsigned int channel)
{
	int size, i;
	unsigned char *src;
	real_t *dst;
	size = image->x * image->y;

	dst = image->data;
	src = bytes + channel;
	for (i = 0; i < size; i++)
	{
		*dst = *src;
		src += 3;
		dst++;
	}
}


/*
 * GET / SET
 */

#if !defined(INLINE) && !defined(INLINE_MACRO)

real_t image_get_mirror(image_t* image, int x, int y)
{
  return MACRO_IMAGE_GET_MIRROR(image, x, y);
}

real_t image_get_period(image_t* image, int x, int y)
{
  return MACRO_IMAGE_GET_PERIOD(image, x, y);
}

void image_set(image_t* image, int x, int y, real_t value)
{
  MACRO_IMAGE_SET(image, x, y, value);
}

real_t image_get(image_t* image, int x, int y)
{
  return MACRO_IMAGE_GET(image, x, y);
}

#endif
