// TagView.h
//
// Part of "tagkit" -- the reusable pieces of TagView.
//
// TagView is a BColumnListView pre-configured to display tagkit::TagRecord
// rows: file name, artist, title, album, track, year, genre, duration and
// format. It has no knowledge of TagLib, MusicBrainz, drag-and-drop
// sources, or anything else app-specific -- it only knows how to render
// TagRecords and hand back the one that's currently selected. That keeps
// it reusable in other apps (Hare, ArmyKnife, ...) that just want a quick
// tag-listing widget.

#ifndef TAGKIT_TAG_VIEW_H
#define TAGKIT_TAG_VIEW_H

#include <ColumnListView.h>
#include <ColumnTypes.h>

#include "TagRecord.h"


namespace tagkit {


// One row in a TagView. Keeps a copy of the TagRecord it was built from so
// callers can retrieve it (e.g. to act on the current selection).
class TagRow : public BRow {
public:
								TagRow(const TagRecord& record);

			const TagRecord&	Record() const { return fRecord; }
			void				SetRecord(const TagRecord& record);

private:
			TagRecord			fRecord;
};


class TagView : public BColumnListView {
public:
								TagView(const char* name);
	virtual						~TagView();

	// Adds a row for the given record and returns it (not owned by the
	// caller -- the view keeps ownership, as usual for BColumnListView).
			TagRow*				AddTag(const TagRecord& record);

	// Removes every row.
			void				Clear();

	// Returns the currently selected row, or NULL if nothing is selected.
	// Callers that need to modify the selection afterward (e.g. applying
	// a MusicBrainz match to it) want this over SelectedRecord() below.
			TagRow*				SelectedRow() const;

	// Returns the record backing the currently selected row, or NULL if
	// nothing is selected.
			const TagRecord*	SelectedRecord() const;

	// Replaces the record for a given row (e.g. once a MusicBrainz match
	// has been applied) and refreshes its displayed fields.
			void				UpdateRow(TagRow* row, const TagRecord& record);

private:
			void				_InitColumns();
			void				_ApplyRecordToRow(TagRow* row,
									const TagRecord& record);
};


} // namespace tagkit

#endif // TAGKIT_TAG_VIEW_H
