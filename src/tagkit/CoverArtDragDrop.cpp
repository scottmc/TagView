/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
#include "CoverArtDragDrop.h"

#include <math.h>
#include <string.h>

#include <AppDefs.h>
#include <Bitmap.h>
#include <Entry.h>
#include <InterfaceDefs.h>
#include <Message.h>
#include <View.h>
#include <Window.h>


namespace tagkit {


namespace {

const float kDragThreshold = 4.0f;		// pixels before a press becomes a drag
const float kThumbnailSize = 96.0f;		// longest side of the drag picture

// Pointer to the view the drag started from, so a drop back on the same
// view can be ignored.
const char* const kSourceViewField = "tagkit:source view";

} // namespace


CoverArtDragDrop::CoverArtDragDrop()
	:
	fDragMessage(NULL),
	fDropMessage(NULL),
	fTracking(false)
{
}


CoverArtDragDrop::~CoverArtDragDrop()
{
	delete fDragMessage;
	delete fDropMessage;
}


void
CoverArtDragDrop::SetDragMessage(BMessage* message)
{
	delete fDragMessage;
	fDragMessage = message;
}


void
CoverArtDragDrop::SetDropMessage(BMessage* message)
{
	delete fDropMessage;
	fDropMessage = message;
}


void
CoverArtDragDrop::MouseDown(BView* view, BPoint where, const BBitmap* shown)
{
	if (fDragMessage == NULL || shown == NULL)
		return;

	fTracking = true;
	fPressPoint = where;

	// Keep getting the pointer's movements even if it leaves the view.
	view->SetMouseEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY);
}


void
CoverArtDragDrop::MouseMoved(BView* view, BPoint where, const BBitmap* shown)
{
	if (!fTracking)
		return;

	BMessage* current = view->Window() != NULL
		? view->Window()->CurrentMessage() : NULL;
	int32 buttons = 0;
	if (current != NULL)
		current->FindInt32("buttons", &buttons);
	if ((buttons & B_PRIMARY_MOUSE_BUTTON) == 0) {
		fTracking = false;
		return;
	}

	if (fabsf(where.x - fPressPoint.x) < kDragThreshold
			&& fabsf(where.y - fPressPoint.y) < kDragThreshold) {
		return;
	}
	fTracking = false;

	if (fDragMessage == NULL || shown == NULL || view->Window() == NULL)
		return;

	BMessage drag(*fDragMessage);
	drag.RemoveName(kSourceViewField);
	drag.AddPointer(kSourceViewField, view);

	// Answers (B_COPY_TARGET) come to the window.
	BBitmap* thumbnail = _Thumbnail(shown);
	if (thumbnail != NULL) {
		BRect bounds = thumbnail->Bounds();
		view->DragMessage(&drag, thumbnail, B_OP_COPY,
			BPoint(bounds.Width() / 2.0f, bounds.Height() / 2.0f),
			view->Window());
	} else
		view->DragMessage(&drag, view->Bounds(), view->Window());
}


void
CoverArtDragDrop::MouseUp()
{
	fTracking = false;
}


BBitmap*
CoverArtDragDrop::_Thumbnail(const BBitmap* shown) const
{
	BRect source = shown->Bounds();
	float width = source.Width() + 1.0f;
	float height = source.Height() + 1.0f;
	float scale = kThumbnailSize / (width > height ? width : height);
	if (scale > 1.0f)
		scale = 1.0f;

	BRect target(0, 0, floorf(width * scale) - 1.0f,
		floorf(height * scale) - 1.0f);
	if (target.Width() < 1.0f || target.Height() < 1.0f)
		return NULL;

	BBitmap* thumbnail = new BBitmap(target, B_BITMAP_ACCEPTS_VIEWS,
		B_RGBA32);
	if (thumbnail->InitCheck() != B_OK) {
		delete thumbnail;
		return NULL;
	}

	BView* canvas = new BView(target, "thumbnailCanvas", B_FOLLOW_NONE, 0);
	thumbnail->AddChild(canvas);
	if (thumbnail->Lock()) {
		canvas->DrawBitmap(shown, source, target, B_FILTER_BITMAP_BILINEAR);
		canvas->Sync();
		thumbnail->Unlock();
	}
	thumbnail->RemoveChild(canvas);
	delete canvas;
	return thumbnail;
}


bool
CoverArtDragDrop::MessageReceived(BView* view, BMessage* message)
{
	if (fDropMessage == NULL || view->Window() == NULL)
		return false;

	// The answer to our request for an image another application offered.
	if (message->what == B_MIME_DATA && !fRequestedType.IsEmpty()) {
		const void* data = NULL;
		ssize_t size = 0;
		if (message->FindData(fRequestedType.String(), B_MIME_TYPE, &data,
				&size) == B_OK && size > 0) {
			_PostDropped(view, fRequestedType.String(), data, size, NULL);
		}
		fRequestedType = "";
		return true;
	}

	if (!message->WasDropped()
			|| (message->what != B_SIMPLE_DATA
				&& message->what != B_REFS_RECEIVED)) {
		return false;
	}

	// Something dragged out of this very view and dropped back on it.
	void* source = NULL;
	if (message->FindPointer(kSourceViewField, &source) == B_OK
			&& source == view) {
		return true;
	}

	// Files dragged from Tracker.
	entry_ref ref;
	if (message->FindRef("refs", &ref) == B_OK) {
		_PostDropped(view, NULL, NULL, 0, &ref);
		return true;
	}

	// An image another application offers: ask for it, preferring the
	// formats the cover art can be stored in as they are.
	static const char* const kPreferred[] = { "image/jpeg", "image/png",
		"image/gif" };
	BString chosen;
	BString type;
	for (int32 i = 0; message->FindString("be:types", i, &type) == B_OK;
			i++) {
		if (!type.StartsWith("image/"))
			continue;

		bool preferred = false;
		for (size_t p = 0; p < sizeof(kPreferred) / sizeof(kPreferred[0]);
				p++) {
			if (type.ICompare(kPreferred[p]) == 0)
				preferred = true;
		}
		if (preferred || chosen.IsEmpty()) {
			chosen = type;
			if (preferred)
				break;
		}
	}
	if (chosen.IsEmpty())
		return false;

	BMessage request(B_COPY_TARGET);
	request.AddString("be:types", chosen.String());
	fRequestedType = chosen;
	message->SendReply(&request, view);
	return true;
}


void
CoverArtDragDrop::_PostDropped(BView* view, const char* mimeType,
	const void* data, ssize_t size, const void* refsSource)
{
	BMessage dropped(*fDropMessage);
	if (refsSource != NULL) {
		dropped.AddRef("refs", static_cast<const entry_ref*>(refsSource));
	} else {
		dropped.AddString("mimeType", mimeType);
		dropped.AddData("imageData", B_RAW_TYPE, data, size);
	}
	view->Window()->PostMessage(&dropped);
}


} // namespace tagkit
