/*
 * Copyright 2003-2019 The Music Player Daemon Project
 * http://www.musicpd.org
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef MPD_DB_PRINT_H
#define MPD_DB_PRINT_H

#include <stdint.h>

enum TagType : uint8_t;
class TagMask;
class SongFilter;
struct DatabaseSelection;
struct RangeArg;
struct Partition;
class Response;

/**
 * @param full print attributes/tags
 * @param base print only base name of songs/directories?
 */
void
db_selection_print(Response &r, Partition &partition,
		   const DatabaseSelection &selection,
		   bool full, bool base);

void
PrintSongUris(Response &r, Partition &partition,
	      const SongFilter *filter);

void
PrintUniqueTags(Response &r, Partition &partition,
		TagType type, TagType group,
		const SongFilter *filter);

#endif
