/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
#include "CoverArtCandidatesView.h"

#include <Bitmap.h>
#include <InterfaceDefs.h>
#include <Message.h>
#include <Rect.h>
#include <ScrollBar.h>
#include <Window.h>


namespace tagkit {


namespace {

const float kThumbnailSize = 96.0f;
const float kThumbnailSpacing = 8.0f;
const float kSelectionBorderWidth = 3.0f;
const float kEndPadding = 3.0f;

// How many thumbnails are visible at once; more than this scroll.
const int32 kVisibleThumbnails = 4;

} // namespace


CoverArtCandidatesView::CoverArtCandidatesView()
	:
	BView("coverArtCandidatesView", B_WILL_DRAW | B_FRAME_EVENTS),
	fCandidates(NULL),
	fCount(0),
	fSelectedIndex(0),
	fSelectionMessage(NULL),
	fInvocationMessage(NULL)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


CoverArtCandidatesView::~CoverArtCandidatesView()
{
	for (int32 i = 0; i < fCount; i++)
		delete fCandidates[i];
	delete[] fCandidates;

	delete fSelectionMessage;
	delete fInvocationMessage;
}


void
CoverArtCandidatesView::SetSelectionMessage(BMessage* message)
{
	delete fSelectionMessage;
	fSelectionMessage = message;
}


void
CoverArtCandidatesView::SetInvocationMessage(BMessage* message)
{
	delete fInvocationMessage;
	fInvocationMessage = message;
}


void
CoverArtCandidatesView::SetCandidates(BBitmap* const* candidates, int32 count)
{
	for (int32 i = 0; i < fCount; i++)
		delete fCandidates[i];
	delete[] fCandidates;
	fCandidates = NULL;
	fCount = 0;

	if (candidates != NULL && count > 0) {
		fCandidates = new BBitmap*[count];
		for (int32 i = 0; i < count; i++)
			fCandidates[i] = candidates[i];
		fCount = count;
	}

	fSelectedIndex = 0;

	_UpdateScrollBar();
	Invalidate();
}


void
CoverArtCandidatesView::Clear()
{
	SetCandidates(NULL, 0);
}


void
CoverArtCandidatesView::AddCandidate(BBitmap* bitmap)
{
	BBitmap** grown = new BBitmap*[fCount + 1];
	for (int32 i = 0; i < fCount; i++)
		grown[i] = fCandidates[i];
	grown[fCount] = bitmap;

	delete[] fCandidates;
	fCandidates = grown;
	fCount++;

	// The strip is a bit longer now; let the scroll bar know.
	_UpdateScrollBar();
	Invalidate();
}


void
CoverArtCandidatesView::AttachedToWindow()
{
	BView::AttachedToWindow();
	_UpdateScrollBar();
}


void
CoverArtCandidatesView::FrameResized(float width, float height)
{
	BView::FrameResized(width, height);
	_UpdateScrollBar();
}


float
CoverArtCandidatesView::_ContentWidth() const
{
	return (fCount * kThumbnailSize) + ((fCount + 1) * kThumbnailSpacing)
		+ (2 * kEndPadding);
}


float
CoverArtCandidatesView::_WidthForThumbnails(int32 count)
{
	return (count * kThumbnailSize) + ((count + 1) * kThumbnailSpacing)
		+ (2 * kEndPadding);
}


void
CoverArtCandidatesView::_UpdateScrollBar()
{
	BScrollBar* bar = ScrollBar(B_HORIZONTAL);
	if (bar == NULL)
		return;

	float viewWidth = Bounds().Width();
	float contentWidth = _ContentWidth();
	float range = contentWidth - viewWidth;
	if (range < 0)
		range = 0;

	bar->SetRange(0, range);
	bar->SetProportion(contentWidth > 0 ? viewWidth / contentWidth : 1.0f);
	bar->SetSteps(kThumbnailSize + kThumbnailSpacing, viewWidth);
}


BSize
CoverArtCandidatesView::MinSize()
{
	return BSize(_WidthForThumbnails(1),
		kThumbnailSize + (2 * kThumbnailSpacing));
}


BSize
CoverArtCandidatesView::PreferredSize()
{
	// Room for kVisibleThumbnails at once, however many candidates there
	// are; the rest are reached with the scroll bar.
	return BSize(_WidthForThumbnails(kVisibleThumbnails),
		kThumbnailSize + (2 * kThumbnailSpacing));
}


BSize
CoverArtCandidatesView::MaxSize()
{
	return BSize(_WidthForThumbnails(kVisibleThumbnails),
		kThumbnailSize + (2 * kThumbnailSpacing));
}


BRect
CoverArtCandidatesView::_ThumbnailRect(int32 index) const
{
	float left = kEndPadding + kThumbnailSpacing
		+ (index * (kThumbnailSize + kThumbnailSpacing));
	float top = kThumbnailSpacing;
	return BRect(left, top, left + kThumbnailSize, top + kThumbnailSize);
}


void
CoverArtCandidatesView::Draw(BRect updateRect)
{
	for (int32 i = 0; i < fCount; i++) {
		BRect rect = _ThumbnailRect(i);
		if (!rect.Intersects(updateRect))
			continue;

		if (fCandidates[i] != NULL) {
			DrawBitmap(fCandidates[i], fCandidates[i]->Bounds(), rect);
		} else {
			SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
			FillRect(rect);
			SetHighColor(ui_color(B_CONTROL_BORDER_COLOR));
			StrokeRect(rect);
		}

		if (i == fSelectedIndex) {
			SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
			SetPenSize(kSelectionBorderWidth);
			StrokeRect(rect.InsetByCopy(-kSelectionBorderWidth / 2.0f,
				-kSelectionBorderWidth / 2.0f));
			SetPenSize(1.0f);
		}
	}
}


void
CoverArtCandidatesView::MouseDown(BPoint where)
{
	for (int32 i = 0; i < fCount; i++) {
		if (!_ThumbnailRect(i).Contains(where))
			continue;

		if (fSelectedIndex != i) {
			fSelectedIndex = i;
			Invalidate();
			if (fSelectionMessage != NULL && Window() != NULL)
				Window()->PostMessage(new BMessage(*fSelectionMessage));
		}

		// Double-click picks it straight away.
		int32 clicks = 1;
		BMessage* current = Window() != NULL ? Window()->CurrentMessage()
			: NULL;
		if (current != NULL)
			current->FindInt32("clicks", &clicks);
		if (clicks >= 2 && fInvocationMessage != NULL && Window() != NULL)
			Window()->PostMessage(new BMessage(*fInvocationMessage));
		break;
	}
}


} // namespace tagkit
