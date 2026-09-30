/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
#include "TagView.h"

#include <stdio.h>


namespace tagkit {


enum {
	kColumnFileName = 0,
	kColumnArtist,
	kColumnTitle,
	kColumnAlbum,
	kColumnTrack,
	kColumnYear,
	kColumnGenre,
	kColumnDuration,
	kColumnFormat
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


// #pragma mark - TagRow


TagRow::TagRow(const TagRecord& record)
	:
	BRow(),
	fRecord(record)
{
}


void
TagRow::SetRecord(const TagRecord& record)
{
	fRecord = record;
}


// #pragma mark - TagView


TagView::TagView(const char* name)
	:
	BColumnListView(name, B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE,
		B_NO_BORDER, true /* showHorizontalScrollbar */)
{
	_InitColumns();
}


TagView::~TagView()
{
}


void
TagView::_InitColumns()
{
	AddColumn(new BStringColumn("File", 160, 60, 400, B_TRUNCATE_MIDDLE),
		kColumnFileName);
	AddColumn(new BStringColumn("Artist", 140, 60, 400, B_TRUNCATE_END),
		kColumnArtist);
	AddColumn(new BStringColumn("Title", 180, 60, 400, B_TRUNCATE_END),
		kColumnTitle);
	AddColumn(new BStringColumn("Album", 160, 60, 400, B_TRUNCATE_END),
		kColumnAlbum);
	AddColumn(new BIntegerColumn("Track", 50, 30, 80, B_ALIGN_RIGHT),
		kColumnTrack);
	AddColumn(new BIntegerColumn("Year", 55, 30, 80, B_ALIGN_RIGHT),
		kColumnYear);
	AddColumn(new BStringColumn("Genre", 100, 60, 200, B_TRUNCATE_END),
		kColumnGenre);
	AddColumn(new BStringColumn("Duration", 70, 50, 100, B_TRUNCATE_END,
		B_ALIGN_RIGHT), kColumnDuration);
	AddColumn(new BStringColumn("Format", 80, 50, 120, B_TRUNCATE_END),
		kColumnFormat);
}


TagRow*
TagView::AddTag(const TagRecord& record)
{
	TagRow* row = new TagRow(record);
	_ApplyRecordToRow(row, record);
	AddRow(row);
	return row;
}


void
TagView::Clear()
{
	// BColumnListView has no single "remove everything" call, so walk the
	// rows from the end and remove/delete each one.
	for (int32 i = CountRows() - 1; i >= 0; i--) {
		BRow* row = RowAt(i);
		RemoveRow(row);
		delete row;
	}
}


TagRow*
TagView::SelectedRow() const
{
	BRow* selected = CurrentSelection();
	if (selected == NULL)
		return NULL;

	return static_cast<TagRow*>(selected);
}


const TagRecord*
TagView::SelectedRecord() const
{
	TagRow* row = SelectedRow();
	if (row == NULL)
		return NULL;

	return &row->Record();
}


void
TagView::UpdateRow(TagRow* row, const TagRecord& record)
{
	if (row == NULL)
		return;

	row->SetRecord(record);
	_ApplyRecordToRow(row, record);

	// Our own UpdateRow() overload hides BColumnListView::UpdateRow(BRow*),
	// so call it explicitly to have the view re-measure/redraw the row.
	BColumnListView::UpdateRow(row);
}


void
TagView::_ApplyRecordToRow(TagRow* row, const TagRecord& record)
{
	row->SetField(new BStringField(record.fileName), kColumnFileName);
	row->SetField(new BStringField(record.artist), kColumnArtist);
	row->SetField(new BStringField(record.title), kColumnTitle);
	row->SetField(new BStringField(record.album), kColumnAlbum);
	row->SetField(new BIntegerField(record.track), kColumnTrack);
	row->SetField(new BIntegerField(record.year), kColumnYear);
	row->SetField(new BStringField(record.genre), kColumnGenre);
	row->SetField(new BStringField(format_duration(record.durationSeconds)),
		kColumnDuration);
	row->SetField(new BStringField(format_label(record.format)),
		kColumnFormat);
}


} // namespace tagkit
