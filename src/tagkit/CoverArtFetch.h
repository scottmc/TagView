/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
// CoverArtFetch.h
//
// Part of "tagkit" -- the reusable pieces of TagView.
// Looks up front cover art for a set of MusicBrainz releases on the Cover
// Art Archive (via libcoverart), the same way Hare's MusicBrainzLookup
// does.

#ifndef TAGKIT_COVER_ART_FETCH_H
#define TAGKIT_COVER_ART_FETCH_H

#include <vector>

#include "CoverArtImage.h"
#include "RecordingMatch.h"


namespace tagkit {

// Fetches each release's front cover (in the given order) until maxImages
// have been found. Releases with no cover art, or whose fetch fails, are
// skipped -- one missing cover doesn't stop the others -- so the result
// can hold fewer images than there were releases, or none at all.
//
// Performs network I/O, one request per release: always call this from a
// worker thread, never the UI thread.
std::vector<CoverArtImage> fetch_cover_art(
	const std::vector<ReleaseRef>& releases, int32 maxImages = 12);

} // namespace tagkit

#endif // TAGKIT_COVER_ART_FETCH_H
