/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 */
#include "RecordingMatchView.h"

#include <stdio.h>


namespace tagkit {


enum {
	kColumnArtist = 0,
	kColumnTitle,
	kColumnAlbum,
	kColumnDuration
};


static BString
format_duration(int32 seconds)
{
	if (seconds < 0)
		return BString();

	int32 minutes = seconds / 60;
	int32 secs = seconds % 60;

	char buffer[16];
	snprintf(buffer, sizeof(buffer), "%" B_PRId32 ":%02" B_PRId32,
		minutes, secs);
	return BString(buffer);
}


// #pragma mark - RecordingMatchRow


RecordingMatchRow::RecordingMatchRow(const RecordingMatch& match)
	:
	BRow(),
	fMatch(match)
{
}


// #pragma mark - RecordingMatchView


RecordingMatchView::RecordingMatchView(const char* name)
	:
	BColumnListView(name, B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE,
		B_NO_BORDER, true /* showHorizontalScrollbar */)
{
	_InitColumns();
}


RecordingMatchView::~RecordingMatchView()
{
}


void
RecordingMatchView::_InitColumns()
{
	AddColumn(new BStringColumn("Artist", 160, 60, 400, B_TRUNCATE_END),
		kColumnArtist);
	AddColumn(new BStringColumn("Title", 200, 60, 400, B_TRUNCATE_END),
		kColumnTitle);
	AddColumn(new BStringColumn("Album", 180, 60, 400, B_TRUNCATE_END),
		kColumnAlbum);
	AddColumn(new BStringColumn("Duration", 70, 50, 100, B_TRUNCATE_END,
		B_ALIGN_RIGHT), kColumnDuration);
}


RecordingMatchRow*
RecordingMatchView::AddMatch(const RecordingMatch& match)
{
	RecordingMatchRow* row = new RecordingMatchRow(match);

	row->SetField(new BStringField(match.artist), kColumnArtist);
	row->SetField(new BStringField(match.title), kColumnTitle);
	row->SetField(new BStringField(match.album), kColumnAlbum);
	row->SetField(new BStringField(format_duration(match.durationSeconds)),
		kColumnDuration);

	AddRow(row);
	return row;
}


void
RecordingMatchView::Clear()
{
	for (int32 i = CountRows() - 1; i >= 0; i--) {
		BRow* row = RowAt(i);
		RemoveRow(row);
		delete row;
	}
}


const RecordingMatch*
RecordingMatchView::SelectedMatch() const
{
	BRow* selected = CurrentSelection();
	if (selected == NULL)
		return NULL;

	RecordingMatchRow* row = static_cast<RecordingMatchRow*>(selected);
	return &row->Match();
}


} // namespace tagkit
