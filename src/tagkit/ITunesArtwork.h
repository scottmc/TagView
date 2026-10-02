/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
// ITunesArtwork.h
//
// Part of "tagkit" -- the reusable pieces of TagView.
// Album cover lookup through Apple's public iTunes Search API
// (https://itunes.apple.com/search), meant as a fallback for when the Cover
// Art Archive has nothing (or nothing that can be fetched) for a release.
// The covers are Apple's, so callers should say where they came from.

#ifndef TAGKIT_ITUNES_ARTWORK_H
#define TAGKIT_ITUNES_ARTWORK_H

#include <String.h>
#include <SupportDefs.h>

#include "CoverArtFetch.h"


namespace tagkit {

// Searches iTunes for albums by artist (and album, if not empty), keeps the
// ones whose artist matches (those whose title matches album first) and
// downloads their cover, calling found() for each as it arrives -- the same
// progressive style as fetch_cover_art(). The images' releaseTitle reads
// "<album> (iTunes)" and releaseId "itunes:<collection id>". Returns how
// many covers were found, at most maxImages. Performs network I/O: call it
// from a worker thread.
int32 fetch_itunes_cover_art(const BString& artist, const BString& album,
	int32 maxImages, const CoverArtFoundFunction& found);

} // namespace tagkit

#endif // TAGKIT_ITUNES_ARTWORK_H
