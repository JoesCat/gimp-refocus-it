/*
 * Written 2003 Lukas Kunc <Lukas.Kunc@seznam.cz>
 * Code for calculation of circle intensity (C) 1999-2003 Ernst Lippe 
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

#include <math.h>
#include <stdlib.h>
#include "blur.h"

#ifndef SQR
#define SQR(x) ((x)*(x))
#endif

typedef struct
{
	real_t x;
	real_t y;
} point_t;


static double circle_integral (double x, double radius)
/* Return the integral of sqrt(radius^2 - z^2) for z = 0 to x */
{
	double sin, sq_diff;
	if (radius < 1e-6)
	{
		return (0.0);
	}
	else
	{
		sin = x / radius;
		sq_diff = SQR (radius) - SQR (x);
		/* From a mathematical point of view the following is redundant.
		Numerically they are not equivalent!
		*/
		if ((sq_diff < 0.0) || (sin < -1.0) || (sin > 1.0))
		{
			if (sin < 0.0)
			{
				return (-0.25 * SQR (radius) * M_PI);
			}
			else
			{
				return (0.25 * SQR (radius) * M_PI);
			}
		}
		else
		{
			return (0.5 * x * sqrt (sq_diff) + 0.5 * SQR (radius) * asin (sin));
		}
	}
}

static double circle_intensity (int x, int y, double radius)
{
	double xlo, xhi, ylo, yhi, xc1, xc2;
	double symmetry_factor;

	if (radius <= 1e-4)
	{
		return (((x == 0) && (y == 0)) ? 1.0 : 0.0);
	}
	else
	{
		xlo = abs (x) - 0.5;
		xhi = abs (x) + 0.5;
		ylo = abs (y) - 0.5;
		yhi = abs (y) + 0.5;
		symmetry_factor = 1.0;

		if (xlo < 0.0)
		{
			xlo = 0.0;
			symmetry_factor *= 2.0;
		};
		if (ylo < 0.0)
		{
			ylo = 0.0;
			symmetry_factor *= 2.0;
		};
		if (SQR (xlo) + SQR (yhi) > SQR (radius))
		{
			xc1 = xlo;
		}
		else if (SQR (xhi) + SQR (yhi) > SQR (radius))
		{
			xc1 = sqrt (SQR (radius) - SQR (yhi));
		}
		else
		{
			xc1 = xhi;
		};
		if (SQR (xlo) + SQR (ylo) > SQR (radius))
		{
			xc2 = xlo;
		}
		else if (SQR (xhi) + SQR (ylo) > SQR (radius))
		{
			xc2 = sqrt (SQR (radius) - SQR (ylo));
		}
		else
		{
			xc2 = xhi;
		};
		return (((yhi - ylo) * (xc1 - xlo) +
			circle_integral (xc2, radius) - circle_integral (xc1, radius) -
			(xc2 - xc1) * ylo) * symmetry_factor / (M_PI * SQR (radius)));
	}
}


/*
 * Create the convolution mask for out-of-focus blur.
 */
convmask_t* blur_create_defocus(convmask_t* blur, real_t radius)
{
	int i, j, r;
	real_t val;

	r = (int)(radius + R(0.5));
	convmask_create(blur, r); //memfull?

	if (r < 1)
	{
		convmask_set(blur, 0, 0, R(1.0));
		return blur;
	}
	else
	{
		for (i = 0; i <= r; i++) {
			for (j = 0; j <= r; j++) {
				val = (real_t)circle_intensity(i, j, radius);
				if (val < R(0.0)) val = R(0.0);
				convmask_set_circle(blur, i, j, val);
			}
		}
		return convmask_normalize(blur);
	}
}

/*
 * Create the convolution mask for gaussian blur.
 */
convmask_t* blur_create_gauss(convmask_t* blur, real_t variance)
{
	double epsilon;
	double mult;
	double var;
	int i, j, radius;

	if (variance < R(1e-6))
	{
		convmask_create(blur, 0); //memfull?
		convmask_set(blur, 0, 0, R(1.0));
		return blur;
	}
	else
	{
		/*
		 * Compute the radius such that the magnitude of central element
		 * is at least 2 times the magnitude of outermost elements.
		 */
		var = (double)variance;
		epsilon = sqrt(-2.0 * log(1e-2));
		radius = (int)(var * epsilon + 0.5);
		convmask_create(blur, radius); //memfull?
		var *= var * 2.0;
		mult = var * M_PI;
		for (i = 0; i <= radius; i++) {
			for (j = 0; j <= radius; j++) {
				convmask_set_circle(blur, i, j, (real_t)(exp(-((double)(i*i + j*j)) / var) / mult));
			}
		}
		return convmask_normalize(blur);
	}
}

static void make_coords_line(point_t* c, real_t r, double a)
{
	real_t sina, cosa;
	real_t x, y;
	int i;

	a *= M_PI / 180.0;
	sina = (real_t)sin(a);
	cosa = (real_t)cos(a);
	c[0].x = -R(0.5);
	c[0].y = R(0.5);
	c[1].x = -R(0.5);
	c[1].y = -R(0.5);
	c[2].x = r + R(0.5);
	c[2].y = -R(0.5);
	c[3].x = r + R(0.5);
	c[3].y = R(0.5);

	for (i = 0; i < 4; i++)
	{
		x = cosa * c[i].x - sina * c[i].y;
		y = sina * c[i].x + cosa * c[i].y;
		c[i].x = x;
		c[i].y = y;
	}
}

static void isect(point_t* isc, point_t* a, point_t* b, real_t x)
{
	real_t t;
	t = (x - a->x) / (b->x - a->x);
	isc->y = -x;
	isc->x = a->y + t * (b->y - a->y);
}

static int clip(point_t* dst, point_t* src, int n, real_t max)
{
	int prev, in, on, i;

	prev = n-1;
	in = (src[prev].x <= max + 1e-4);
	on = 0;

	for (i = 0; i < n; prev = i++)
	{
		if (src[i].x <= max + 1e-4)
		{
			/* current is in */
			if (!in)
			{
				/* previous was out - need to compute intersection */
				isect(dst + (on++), src + i, src + prev, max);
			}
			dst[on].x = src[i].y;
			dst[on++].y = -src[i].x;
			in = 1;
		}
		else
		{
			/* current is out */
			if (in)
			{
				isect(dst + (on++), src + prev, src + i, max);
				/* previous was in - need to compute intersection */
			}
			in = 0;
		}
	}
	return on;
}

/*
 * find intersection of the unit square with center at [x,y] and the rectangle in src
 */
static int find_intersect(point_t* dst, point_t* src, real_t x, real_t y)
{
	int n;
	point_t p[8];

	/* initially there are 4 points in the polygon being clipped */
	n = 4;

	/* clip against x + 0.5 */
	n = clip(p, src, n, x + R(0.5));
	if (n <= 0) return 0;

	/* clip against y + 0.5 */
	n = clip(dst, p, n, y + R(0.5));
	if (n <= 0) return 0;

	/* clip against x - 0.5 */
	n = clip(p, dst, n, -(x - R(0.5)));
	if (n <= 0) return 0;

	/* clip against y - 0.5 */
	n = clip(dst, p, n, -(y - R(0.5)));
	return n;
}

static real_t compute_area(point_t* poly, int n)
{
	real_t sum;
	int prev, i;

	sum = R(0.0);
	for (i = 0, prev = n-1; i < n; prev=i++)
	{
		sum += (poly[i].x - poly[prev].x) * (poly[i].y + poly[prev].y);
	}
	if (sum < R(0.0)) sum = -sum;
	return sum / R(2.0);
}

static real_t intersect_area(real_t x, real_t y, point_t* c)
{
	point_t isec[8];
	int verts;
	verts = find_intersect(isec, c, x, y);
	return compute_area(isec, verts);
}

/*
 * Create the convolution mask for motion blur.
 */
convmask_t* blur_create_motion(convmask_t* blur, real_t radius, real_t angle)
{
	int i, j, r;
	point_t coords_line[4];

	if (radius < R(1e-4))
	{
		convmask_create(blur, 0.01); //memfull?
		convmask_set(blur, 0, 0, R(1.0));
		return blur;
	}
	else
	{
		r = (int)(radius + R(1.0));
		convmask_create(blur, r); //memfull?
		make_coords_line(coords_line, radius, angle);
		for (i = -r; i <= r; i++) {
			for (j = -r; j <= r; j++) {
				convmask_set(blur, i, j, intersect_area((real_t)i, (real_t)j, coords_line));
			}
		}
		return convmask_normalize(blur);
	}
}
