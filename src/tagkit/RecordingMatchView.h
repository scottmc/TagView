/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
// RecordingMatchView.h
//
// Part of "tagkit" -- the reusable pieces of TagView.
// A BColumnListView preconfigured to show MusicBrainz recording search
// results (artist/title/album/duration, in the relevance order
// MusicBrainz itself returned them) for the user to pick from. Sibling
// to tagkit::TagView -- kept as its own class since the columns that
// make sense for a search result (no file name/track/year/genre/
// format) don't match a tag listing's.

#ifndef TAGKIT_RECORDING_MATCH_VIEW_H
#define TAGKIT_RECORDING_MATCH_VIEW_H

#include <ColumnListView.h>
#include <ColumnTypes.h>

#include "RecordingMatch.h"


namespace tagkit {


class RecordingMatchRow : public BRow {
public:
								RecordingMatchRow(const RecordingMatch& match);

			const RecordingMatch&	Match() const { return fMatch; }

private:
			RecordingMatch		fMatch;
};


class RecordingMatchView : public BColumnListView {
public:
								RecordingMatchView(const char* name);
	virtual						~RecordingMatchView();

			RecordingMatchRow*	AddMatch(const RecordingMatch& match);
			void				Clear();
			const RecordingMatch*	SelectedMatch() const;

private:
			void				_InitColumns();
};


} // namespace tagkit

#endif // TAGKIT_RECORDING_MATCH_VIEW_H
