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
// grows the squares and the text with it.

#ifndef TAGKIT_COMPACT_VIEW_H
#define TAGKIT_COMPACT_VIEW_H

#include <View.h>

class BBitmap;


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

private:
			void				_LayoutSquares();

			CompactInfoSquare*	fInfoSquare;
			CompactArtSquare*	fArtSquare;
};

} // namespace tagkit

#endif // TAGKIT_COMPACT_VIEW_H
