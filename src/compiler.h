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

#ifndef _COMPILER_CONF_H
#define _COMPILER_CONF_H

#ifdef HAVE_CONFIG_H
#include "refocus-it-config.h"
#endif

#include <math.h>

#ifdef USE_DOUBLES

typedef double       real_t;
#define R(x)         (x)

#else

typedef float        real_t;
#define R(x)         (x##F)

#endif

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

#ifdef USE_INLINES

#undef INLINE_MACRO
#ifndef INLINE
#if defined(MSC_VER)
#define INLINE __inline
#elif defined(inline) || defined(__cplusplus)
#define INLINE inline
#endif
#endif

#else

#undef INLINE
#define INLINE_MACRO

#endif

#ifdef __cplusplus
#define C_DECL_BEGIN extern "C" {
#define C_DECL_END }
#else
#define C_DECL_BEGIN
#define C_DECL_END
#endif

#ifdef USE_BACKSLASH
#define OS_SLASH '\\'
#else
#define OS_SLASH '/'
#endif

#endif

