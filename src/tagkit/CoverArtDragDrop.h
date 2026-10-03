/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
// CoverArtDragDrop.h
//
// Part of "tagkit" -- the reusable pieces of TagView.
// Drag and drop for a view that shows one cover image, the way ShowImage
// does it (Be's "negotiated" drag and drop):
//
// Dragging out: pressing the primary button on the image and moving a few
// pixels drags the whole image, as a small thumbnail, carrying the drag
// message given to SetDragMessage() (its be:types, be:filetypes and
// be:clip_name say what it can become). Whoever it is dropped on (Tracker,
// the desktop, another application) answers with a B_COPY_TARGET message,
// which arrives at the view's window -- the owner of the image answers
// that, since the view itself never holds the image's bytes.
//
// Dropping in: an image file dragged from Tracker (a "refs" message), or an
// image offered by another application ("be:types"), is taken from the drop
// and handed to the window as a copy of the message given to
// SetDropMessage(), with either the file ("refs") or the image bytes
// ("imageData" with its "mimeType") added.
//
// A view owns one of these and forwards its MouseDown(), MouseMoved(),
// MouseUp() and MessageReceived() to it.

#ifndef TAGKIT_COVER_ART_DRAG_DROP_H
#define TAGKIT_COVER_ART_DRAG_DROP_H

#include <Point.h>
#include <Rect.h>
#include <String.h>

class BBitmap;
class BMessage;
class BView;


namespace tagkit {

class CoverArtDragDrop {
public:
								CoverArtDragDrop();
								~CoverArtDragDrop();

	// The message dragged out of the view (NULL: nothing can be dragged
	// out). It should be a B_SIMPLE_DATA message. Takes ownership.
			void				SetDragMessage(BMessage* message);

	// Posted (as a copy, with the dropped image added) to the view's window
	// when an image is dropped on the view. Takes ownership.
			void				SetDropMessage(BMessage* message);

	// For a primary-button press on the image; shown is the bitmap being
	// displayed (NULL if none, which means there's nothing to drag).
			void				MouseDown(BView* view, BPoint where,
									const BBitmap* shown);
			void				MouseMoved(BView* view, BPoint where,
									const BBitmap* shown);
			void				MouseUp();

	// Returns true if message was a drop (or the answer to a request for
	// the dropped image) and has been dealt with.
			bool				MessageReceived(BView* view, BMessage* message);

private:
			BBitmap*			_Thumbnail(const BBitmap* shown) const;
			void				_PostDropped(BView* view,
									const char* mimeType, const void* data,
									ssize_t size, const void* refsSource);

			BMessage*			fDragMessage;
			BMessage*			fDropMessage;
			BPoint				fPressPoint;
			bool				fTracking;
			BString				fRequestedType;
};

} // namespace tagkit

#endif // TAGKIT_COVER_ART_DRAG_DROP_H
