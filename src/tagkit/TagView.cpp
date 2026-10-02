/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
#include "TagView.h"

#include "OptionalIntegerColumn.h"

#include <Message.h>
#include <Window.h>

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
	kColumnFormat,
	kColumnCover
};


namespace {

// The editable columns: same as the plain ones, but a right-click on a
// cell is reported to the TagView (see TagView::SetEditMessage()) instead
// of being ignored.
void
report_right_click(BColumnListView* parent, BRow* row, tag_field field,
	const BRect& fieldRect)
{
	TagView* view = static_cast<TagView*>(parent);
	view->ColumnRightClicked(static_cast<TagRow*>(row), field,
		fieldRect.Width());
}


class EditableStringColumn : public BStringColumn {
public:
	EditableStringColumn(const char* title, float width, float minWidth,
		float maxWidth, uint32 truncate, tag_field field)
		:
		BStringColumn(title, width, minWidth, maxWidth, truncate),
		fField(field)
	{
		SetWantsEvents(true);
	}

	virtual void MouseDown(BColumnListView* parent, BRow* row, BField* field,
		BRect fieldRect, BPoint point, uint32 buttons)
	{
		if ((buttons & B_SECONDARY_MOUSE_BUTTON) != 0) {
			report_right_click(parent, row, fField, fieldRect);
			return;
		}
		BStringColumn::MouseDown(parent, row, field, fieldRect, point,
			buttons);
	}

private:
	tag_field	fField;
};


class EditableIntegerColumn : public OptionalIntegerColumn {
public:
	EditableIntegerColumn(const char* title, float width, float minWidth,
		float maxWidth, tag_field field)
		:
		OptionalIntegerColumn(title, width, minWidth, maxWidth),
		fField(field)
	{
		SetWantsEvents(true);
	}

	virtual void MouseDown(BColumnListView* parent, BRow* row, BField* field,
		BRect fieldRect, BPoint point, uint32 buttons)
	{
		if ((buttons & B_SECONDARY_MOUSE_BUTTON) != 0) {
			report_right_click(parent, row, fField, fieldRect);
			return;
		}
		OptionalIntegerColumn::MouseDown(parent, row, field, fieldRect, point,
			buttons);
	}

private:
	tag_field	fField;
};

} // namespace


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
		B_NO_BORDER, true /* showHorizontalScrollbar */),
	fEditMessage(NULL),
	fSelectionChangedMessage(NULL)
{
	_InitColumns();
}


TagView::~TagView()
{
	delete fSelectionChangedMessage;
	delete fEditMessage;
}


void
TagView::SetEditMessage(BMessage* message)
{
	delete fEditMessage;
	fEditMessage = message;
}


void
TagView::ColumnRightClicked(TagRow* row, tag_field field, float cellWidth)
{
	if (row == NULL || fEditMessage == NULL || Window() == NULL)
		return;

	// The edit is for the row that was clicked, so make that the selection.
	if (!row->IsSelected()) {
		DeselectAll();
		AddToSelection(row);
	}

	BPoint where;
	uint32 buttons;
	GetMouse(&where, &buttons, false);
	ConvertToScreen(&where);

	BMessage* request = new BMessage(*fEditMessage);
	request->AddInt32("field", (int32)field);
	request->AddPoint("where", where);
	request->AddFloat("width", cellWidth);
	request->AddPointer("row", row);
	Window()->PostMessage(request);
}


void
TagView::SetSelectionChangedMessage(BMessage* message)
{
	delete fSelectionChangedMessage;
	fSelectionChangedMessage = message;
}


void
TagView::SelectionChanged()
{
	BColumnListView::SelectionChanged();

	// Same approach as Hare's EncoderListView: tell the window.
	if (fSelectionChangedMessage != NULL && Window() != NULL)
		Window()->PostMessage(new BMessage(*fSelectionChangedMessage));
}


void
TagView::_InitColumns()
{
	AddColumn(new BStringColumn("File", 160, 60, 400, B_TRUNCATE_MIDDLE),
		kColumnFileName);
	AddColumn(new EditableStringColumn("Artist", 140, 60, 400, B_TRUNCATE_END,
		TAG_FIELD_ARTIST), kColumnArtist);
	AddColumn(new EditableStringColumn("Title", 180, 60, 400, B_TRUNCATE_END,
		TAG_FIELD_TITLE), kColumnTitle);
	AddColumn(new EditableStringColumn("Album", 160, 60, 400, B_TRUNCATE_END,
		TAG_FIELD_ALBUM), kColumnAlbum);
	AddColumn(new EditableIntegerColumn("Track", 50, 30, 80, TAG_FIELD_TRACK),
		kColumnTrack);
	AddColumn(new EditableIntegerColumn("Year", 55, 30, 80, TAG_FIELD_YEAR),
		kColumnYear);
	AddColumn(new EditableStringColumn("Genre", 100, 60, 200, B_TRUNCATE_END,
		TAG_FIELD_GENRE), kColumnGenre);
	AddColumn(new BStringColumn("Duration", 70, 50, 100, B_TRUNCATE_END,
		B_ALIGN_RIGHT), kColumnDuration);
	AddColumn(new BStringColumn("Format", 80, 50, 120, B_TRUNCATE_END),
		kColumnFormat);
	AddColumn(new BStringColumn("Cover", 55, 40, 100, B_TRUNCATE_END),
		kColumnCover);
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
	// A leading bullet marks a row with changes not yet saved to the file.
	BString fileName;
	if (record.modified)
		fileName << "\xE2\x80\xA2 ";
	fileName << record.fileName;
	row->SetField(new BStringField(fileName), kColumnFileName);
	row->SetField(new BStringField(record.artist), kColumnArtist);
	row->SetField(new BStringField(record.title), kColumnTitle);
	row->SetField(new BStringField(record.album), kColumnAlbum);
	row->SetField(new BStringField(format_optional_int(record.track)),
		kColumnTrack);
	row->SetField(new BStringField(format_optional_int(record.year)),
		kColumnYear);
	row->SetField(new BStringField(record.genre), kColumnGenre);
	row->SetField(new BStringField(format_duration(record.durationSeconds)),
		kColumnDuration);
	row->SetField(new BStringField(format_label(record.format)),
		kColumnFormat);

	// "New" = art chosen but not saved to the file yet.
	row->SetField(new BStringField(record.newCoverArt != NULL ? "New"
		: record.hasCoverArt ? "Yes" : ""), kColumnCover);
}


} // namespace tagkit
