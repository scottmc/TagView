/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
// CoverArtCandidatesView.h
//
// Part of "tagkit" -- the reusable pieces of TagView.
// Adapted from Hare's CoverArtCandidatesView: a horizontally scrollable
// row of cover art thumbnails to choose between. Clicking a thumbnail
// marks it as selected with a bold border; the selection starts on the
// first candidate.
//
// Unlike Hare's, it tells its window when the selection changes or a
// thumbnail is double-clicked, by posting the messages given to
// SetSelectionMessage()/SetInvocationMessage() to Window(), so the window
// can show details (album title...) for the selection or accept it.
//
// Meant to live inside a horizontal-only BScrollView: it reports its own
// natural width via PreferredSize(), sized to fit every candidate in a
// single row, so the scroll view knows how far there is to scroll.

#ifndef TAGKIT_COVER_ART_CANDIDATES_VIEW_H
#define TAGKIT_COVER_ART_CANDIDATES_VIEW_H

#include <View.h>

class BBitmap;
class BMessage;


namespace tagkit {

class CoverArtCandidatesView : public BView {
public:
								CoverArtCandidatesView();
	virtual						~CoverArtCandidatesView();

	virtual	void				Draw(BRect updateRect);
	virtual	void				MouseDown(BPoint where);
	virtual	BSize				MinSize();
	virtual	BSize				PreferredSize();
	virtual	BSize				MaxSize();

	// Posted to Window() whenever the selected thumbnail changes / when a
	// thumbnail is double-clicked. The view takes ownership of the
	// messages.
			void				SetSelectionMessage(BMessage* message);
			void				SetInvocationMessage(BMessage* message);

	// Takes ownership of every non-NULL bitmap in candidates, replacing
	// (and freeing) whatever was shown before, and resets the selection to
	// the first candidate. A NULL entry is a candidate that didn't decode:
	// it still gets a placeholder slot so positions keep lining up with
	// the caller's own parallel list of candidates.
			void				SetCandidates(BBitmap* const* candidates,
									int32 count);
			void				Clear();

			int32				SelectedIndex() const
									{ return fSelectedIndex; }
			int32				CountCandidates() const { return fCount; }

private:
			BRect				_ThumbnailRect(int32 index) const;

			BBitmap**			fCandidates;
			int32				fCount;
			int32				fSelectedIndex;
			BMessage*			fSelectionMessage;
			BMessage*			fInvocationMessage;
};

} // namespace tagkit

#endif // TAGKIT_COVER_ART_CANDIDATES_VIEW_H
