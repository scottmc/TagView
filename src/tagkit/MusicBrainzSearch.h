/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
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
	// Without a targetSeconds (< 0) matches come back in MusicBrainz's own
	// relevance order. With one, everything MusicBrainz returned is
	// re-sorted by how close its length is to targetSeconds (closest
	// first, ties keeping MusicBrainz's relevance order, recordings with
	// no known length last) before being cut down to maxResults -- the
	// closer the length, the likelier it's the same recording.
	static std::vector<RecordingMatch> SearchRecording(const BString& artist,
		const BString& song, int32 maxResults = 10, int32 targetSeconds = -1);
};

} // namespace tagkit

#endif // TAGKIT_MUSIC_BRAINZ_SEARCH_H
