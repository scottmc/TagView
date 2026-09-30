/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
// RecordingMatch.h
//
// Part of "tagkit" -- the reusable pieces of TagView.
// One MusicBrainz recording (track) search result. Deliberately has no
// dependency on libmusicbrainz5's own types, so callers (and other apps)
// don't need to include/link against it just to hold or display a
// search result.

#ifndef TAGKIT_RECORDING_MATCH_H
#define TAGKIT_RECORDING_MATCH_H

#include <String.h>
#include <SupportDefs.h>


namespace tagkit {

struct RecordingMatch {
	RecordingMatch()
		:
		durationSeconds(-1),
		score(-1)
	{
	}

	BString	id;			// MusicBrainz recording MBID
	BString	title;
	BString	artist;
	BString	album;			// best effort: the first release this
							// recording appears on, if any
	int32	durationSeconds;	// -1 == unknown
	int32	score;			// 0-100 relevance MusicBrainz assigned to
							// this result, or -1 if unknown/unavailable.
							// tagkit::MusicBrainzSearch currently always
							// leaves this at -1 -- see its SearchRecording()
							// for why -- but matches still come back in
							// MusicBrainz's own relevance order.
};

} // namespace tagkit

#endif // TAGKIT_RECORDING_MATCH_H
