/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
// CompactView.h
//
// Part of "tagkit" -- the reusable pieces of TagView.
// A compact alternative to the column list for showing one file: an
// outlined rectangle holding two squares side by side. The left square
// lists the tag fields, one per row, each with its name in bold followed by
// its value; the right square shows the cover art, or a blue gradient when
// there isn't any. Everything scales with the view, so growing the window
// grows the squares and the text with it. The file's name is shown under
// the squares, inside the outline, so things in it (a track number, say)
// can be copied into the tags by hand.

#ifndef TAGKIT_COMPACT_VIEW_H
#define TAGKIT_COMPACT_VIEW_H

#include <String.h>
#include <View.h>

class BBitmap;
class BMessage;


namespace tagkit {

struct TagRecord;
class CompactInfoSquare;
class CompactArtSquare;


class CompactView : public BView {
public:
								CompactView(const char* name);
	virtual						~CompactView();

	virtual	void				AttachedToWindow();
	virtual	void				FrameResized(float width, float height);
	virtual	void				Draw(BRect updateRect);

	// Shows the tag fields of record (copied, so it needn't outlive the
	// call); NULL shows an empty "no file selected" state.
			void				SetRecord(const TagRecord* record);

	// Takes ownership of bitmap (NULL shows the blue gradient), freeing
	// whatever was shown before.
			void				SetCoverBitmap(BBitmap* bitmap);

	// Posted to Window() when the user right-clicks the Artist, Title,
	// Album, Track, Year or Genre row. The posted copy carries "field" (a
	// tag_field, int32), "where" (the pointer's screen position, a BPoint)
	// and "width" (the tag square's width, a float), so whatever edits the
	// field can open at the row. The view takes ownership of the message.
			void				SetEditMessage(BMessage* message);

private:
	friend class CompactInfoSquare;

			void				_RequestEdit(int32 field, BPoint screenWhere,
									float width);
			void				_LayoutSquares();
			float				_FileNameStripHeight() const;

			CompactInfoSquare*	fInfoSquare;
			CompactArtSquare*	fArtSquare;
			BMessage*			fEditMessage;
			BString				fFileName;
};

} // namespace tagkit

#endif // TAGKIT_COMPACT_VIEW_H
