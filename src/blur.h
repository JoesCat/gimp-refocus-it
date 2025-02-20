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

#ifndef _BLUR_H
#define _BLUR_H

#include "compiler.h"
#include "convmask.h"

C_DECL_BEGIN

convmask_t* blur_create_defocus(convmask_t* blur, double radius);
convmask_t* blur_create_gauss(convmask_t* blur, double variance);
convmask_t* blur_create_motion(convmask_t* blur, double radius, double angle);

C_DECL_END

#endif
