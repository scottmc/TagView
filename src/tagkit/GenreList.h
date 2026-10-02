/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
// GenreList.h
//
// Part of "tagkit" -- the reusable pieces of TagView.
// The standard music genre names (the ID3v1 list plus the Winamp
// additions), taken from Hare's GenreList (Hare Team, MIT License) so both
// apps offer the same choices -- spellings included. Meant for filling a
// genre drop-down menu.

#ifndef TAGKIT_GENRE_LIST_H
#define TAGKIT_GENRE_LIST_H

#include <vector>

#include <String.h>


namespace tagkit {

// Every genre name, sorted alphabetically (ignoring case) for a menu.
// Built on first use; the same list is returned every time.
const std::vector<BString>& genre_names();

} // namespace tagkit

#endif // TAGKIT_GENRE_LIST_H
