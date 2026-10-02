/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
// MusicBrainzSearch.h
//
// Part of "tagkit" -- the reusable pieces of TagView.
// A synchronous MusicBrainz recording (track) search by artist + title,
// wrapping libmusicbrainz5 the same way Hare's MusicBrainzLookup does
// (see src/Hare/MusicBrainzLookup.cpp there) so other apps can reuse
// this instead of re-deriving the query/parsing boilerplate.

#ifndef TAGKIT_MUSIC_BRAINZ_SEARCH_H
#define TAGKIT_MUSIC_BRAINZ_SEARCH_H

#include <vector>

#include <String.h>

#include "RecordingMatch.h"


namespace tagkit {

class MusicBrainzSearch {
public:
	// Performs network I/O -- always call this from a worker thread,
	// never the UI thread. Returns up to maxResults matches, or an empty
	// vector if there were no matches or the lookup failed for any reason
	// (network error, malformed response, ...) -- failures are swallowed
	// rather than thrown, so a failed search just looks like zero results.
	//
	// album is optional (pass an empty string to leave it out): when given
	// it's added to the query so only recordings that appear on a matching
	// release come back, and that release is the one reported as each
	// match's album/year/track (and listed first among its releases for
	// cover art). Handy when the plain artist + song search buries the
	// album you're after among live versions and compilations.
	//
	// Without a targetSeconds (< 0) matches come back in MusicBrainz's own
	// relevance order. With one, everything MusicBrainz returned is
	// re-sorted by how close its length is to targetSeconds (closest
	// first, ties keeping MusicBrainz's relevance order, recordings with
	// no known length last) before being cut down to maxResults -- the
	// closer the length, the likelier it's the same recording.
	static std::vector<RecordingMatch> SearchRecording(const BString& artist,
		const BString& song, const BString& album, int32 maxResults = 10,
		int32 targetSeconds = -1);

	// For finding cover art without a song: releases by artist, narrowed
	// to titles matching album if one is given (pass an empty string to
	// leave it out), in MusicBrainz's relevance order and with repeats of
	// the same title and year dropped. Same rules as SearchRecording():
	// network I/O on a worker thread, failures look like an empty result.
	static std::vector<ReleaseRef> SearchReleases(const BString& artist,
		const BString& album, int32 maxResults = 25);
};

} // namespace tagkit

#endif // TAGKIT_MUSIC_BRAINZ_SEARCH_H
