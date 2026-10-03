/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
// CoverArtView.h
//
// Part of "tagkit" -- the reusable pieces of TagView.
// A small square view that shows one cover art bitmap, scaled to fit with
// its aspect ratio kept, or a plain "No cover art" placeholder when there
// isn't one. A simpler cousin of Hare's CoverArtView (no CD button), meant
// to sit under a tag list showing the art of the selected file.

#ifndef TAGKIT_COVER_ART_VIEW_H
#define TAGKIT_COVER_ART_VIEW_H

#include <View.h>

#include "CoverArtDragDrop.h"

class BBitmap;
class BMessage;


namespace tagkit {

class CoverArtView : public BView {
public:
								CoverArtView(const char* name,
									float size = 112.0f);
	virtual						~CoverArtView();

	virtual	void				Draw(BRect updateRect);
	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 transit,
									const BMessage* dragMessage);
	virtual	void				MouseUp(BPoint where);
	virtual	void				MessageReceived(BMessage* message);

	// Drag and drop of the whole image (see CoverArtDragDrop): the message
	// dragged out of the view (NULL: not draggable), and the message posted
	// to Window() -- with the dropped image added -- when one is dropped on
	// the view. The view takes ownership of both.
			void				SetDragMessage(BMessage* message);
			void				SetDropMessage(BMessage* message);

	// Posted to Window() when the user right-clicks the view, with the
	// pointer's screen position added as "where" (a BPoint), so the window
	// can open a context menu there. The view takes ownership.
			void				SetContextMessage(BMessage* message);

	// Takes ownership of bitmap (NULL shows the placeholder), freeing
	// whatever was shown before.
			void				SetBitmap(BBitmap* bitmap);
			void				Clear();

private:
			BBitmap*			fBitmap;
			BMessage*			fContextMessage;
			CoverArtDragDrop	fDragDrop;
};

} // namespace tagkit

#endif // TAGKIT_COVER_ART_VIEW_H
