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

	// Makes the text in the tag square larger (> 1) or smaller (< 1) than
	// its automatic size, which scales with the square. Limited to 0.5-2.0;
	// the default is 1.0. A value too long for its row wraps onto one extra
	// line, and is cut off with an ellipsis if that isn't enough. When the
	// wrapped rows don't fit, the text shrinks until they do.
			void				SetTextScale(float scale);
			float				TextScale() const { return fTextScale; }

	// Drag and drop of the whole cover image, as for CoverArtView: the
	// message dragged out of the art square (NULL: not draggable) and the
	// message posted to Window(), with the dropped image added, when one is
	// dropped on it. The view takes ownership of both.
			void				SetCoverDragMessage(BMessage* message);
			void				SetCoverDropMessage(BMessage* message);

	// Like the cover art view's: posted to Window() (with the pointer's
	// screen position as "where") when the cover art square is
	// right-clicked. The view takes ownership of the message.
			void				SetCoverContextMessage(BMessage* message);

	// Posted to Window() when the user right-clicks the Artist, Title,
	// Album, Track, Year or Genre row. The posted copy carries "field" (a
	// tag_field, int32), "where" (the pointer's screen position, a BPoint)
	// and "width" (the tag square's width, a float), so whatever edits the
	// field can open at the row. The view takes ownership of the message.
			void				SetEditMessage(BMessage* message);

private:
	friend class CompactInfoSquare;
	friend class CompactArtSquare;

			void				_RequestEdit(int32 field, BPoint screenWhere,
									float width);
			void				_RequestCoverMenu(BPoint screenWhere);
			void				_LayoutSquares();
			float				_FileNameStripHeight() const;

			CompactInfoSquare*	fInfoSquare;
			CompactArtSquare*	fArtSquare;
			BMessage*			fEditMessage;
			BMessage*			fCoverContextMessage;
			BString				fFileName;
			float				fTextScale;
};

} // namespace tagkit

#endif // TAGKIT_COMPACT_VIEW_H
