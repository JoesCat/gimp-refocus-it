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

#include <stdio.h>
#include <string.h>
#include "xmalloc.h"

void* xmalloc(size_t size)
{
	void *mem;
	mem = malloc(size);
	if (!mem) {
		fprintf(stderr, "Allocation of %lu bytes failed\n",
			(unsigned long)size);
		exit(1);
	}
	return mem;
}

void xfree(void* mem)
{
	free(mem);
}

char* xstrdup(const char* str)
{
	char* ret;
	if (!str) return NULL;
	ret = xmalloc(strlen(str) + 1);
	strcpy(ret, str);
	return ret;
}


