/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
#include "CoverArtView.h"

#include <Bitmap.h>
#include <InterfaceDefs.h>
#include <Message.h>
#include <Rect.h>
#include <Size.h>
#include <String.h>
#include <Window.h>


namespace tagkit {


CoverArtView::CoverArtView(const char* name, float size)
	:
	BView(name, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
	fBitmap(NULL),
	fContextMessage(NULL)
{
	SetViewColor(B_TRANSPARENT_COLOR);

	BSize square(size, size);
	SetExplicitMinSize(square);
	SetExplicitPreferredSize(square);
	SetExplicitMaxSize(square);
}


CoverArtView::~CoverArtView()
{
	delete fBitmap;
	delete fContextMessage;
}


void
CoverArtView::SetContextMessage(BMessage* message)
{
	delete fContextMessage;
	fContextMessage = message;
}


void
CoverArtView::MouseDown(BPoint where)
{
	BMessage* current = Window() != NULL ? Window()->CurrentMessage() : NULL;
	int32 buttons = 0;
	if (fContextMessage == NULL || current == NULL
			|| current->FindInt32("buttons", &buttons) != B_OK
			|| (buttons & B_SECONDARY_MOUSE_BUTTON) == 0) {
		BView::MouseDown(where);
		return;
	}

	BMessage* request = new BMessage(*fContextMessage);
	request->AddPoint("where", ConvertToScreen(where));
	Window()->PostMessage(request);
}


void
CoverArtView::SetBitmap(BBitmap* bitmap)
{
	if (bitmap != fBitmap) {
		delete fBitmap;
		fBitmap = bitmap;
	}
	Invalidate();
}


void
CoverArtView::Clear()
{
	SetBitmap(NULL);
}


void
CoverArtView::Draw(BRect updateRect)
{
	BRect bounds = Bounds();

	SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	FillRect(bounds);

	if (fBitmap != NULL) {
		// Fit inside the view keeping the image's aspect ratio, centered.
		BRect source = fBitmap->Bounds();
		float scale = bounds.Width() / (source.Width() + 1.0f);
		float heightScale = bounds.Height() / (source.Height() + 1.0f);
		if (heightScale < scale)
			scale = heightScale;

		float width = (source.Width() + 1.0f) * scale;
		float height = (source.Height() + 1.0f) * scale;
		BRect target(0, 0, width - 1.0f, height - 1.0f);
		target.OffsetBy(bounds.left + (bounds.Width() + 1.0f - width) / 2.0f,
			bounds.top + (bounds.Height() + 1.0f - height) / 2.0f);

		SetDrawingMode(B_OP_ALPHA);
		SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
		DrawBitmap(fBitmap, source, target, B_FILTER_BITMAP_BILINEAR);
		SetDrawingMode(B_OP_COPY);
		return;
	}

	// Placeholder: an outlined square with a short label.
	SetHighColor(ui_color(B_CONTROL_BORDER_COLOR));
	StrokeRect(bounds);

	const char* label = "No cover art";
	SetHighColor(tint_color(ui_color(B_PANEL_TEXT_COLOR), B_LIGHTEN_1_TINT));
	font_height fontHeight;
	GetFontHeight(&fontHeight);
	BPoint point(
		bounds.left + (bounds.Width() - StringWidth(label)) / 2.0f,
		bounds.top + (bounds.Height() + fontHeight.ascent
			- fontHeight.descent) / 2.0f);
	DrawString(label, point);
}


} // namespace tagkit
