/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
#include "Barberpole.h"

#include <InterfaceDefs.h>
#include <Window.h>


Barberpole::Barberpole(const char* name, uint32 flags, const char* idleText)
	:
	BBox(name, flags),
	fIsRunning(false),
	fBitmap(NULL),
	fIdleText(idleText)
{
	fPattern.data[0] = 0x0f;
	fPattern.data[1] = 0x1e;
	fPattern.data[2] = 0x3c;
	fPattern.data[3] = 0x78;
	fPattern.data[4] = 0xf0;
	fPattern.data[5] = 0xe1;
	fPattern.data[6] = 0xc3;
	fPattern.data[7] = 0x87;

	_CreateBitmap();

	SetFont(be_plain_font);
}


Barberpole::~Barberpole()
{
	delete fBitmap;
}


void
Barberpole::Start()
{
	fIsRunning = true;
	Window()->SetPulseRate(100000);
	SetFlags(Flags() | B_PULSE_NEEDED);
	SetViewColor(B_TRANSPARENT_COLOR);
	Invalidate();
}


void
Barberpole::Pause()
{
	Window()->SetPulseRate(500000);
	SetFlags(Flags() & ~B_PULSE_NEEDED);
	Invalidate();
}


void
Barberpole::Stop()
{
	fIsRunning = false;
	Window()->SetPulseRate(500000);
	SetFlags(Flags() & ~B_PULSE_NEEDED);
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	Invalidate();
}


void
Barberpole::Pulse()
{
	uchar tmp = fPattern.data[7];

	for (int j = 7; j > 0; --j)
		fPattern.data[j] = fPattern.data[j - 1];

	fPattern.data[0] = tmp;

	Invalidate();
}


void
Barberpole::Draw(BRect updateRect)
{
	if (IsRunning()) {
		_DrawOnBitmap();
		SetDrawingMode(B_OP_COPY);
		DrawBitmap(fBitmap);
		return;
	}

	BBox::Draw(updateRect);

	BFont font;
	GetFont(&font);

	float stringWidth = StringWidth(fIdleText.String());
	float stringHeight = font.Size();

	BRect bounds = Bounds();

	DrawString(fIdleText.String(),
		BPoint(bounds.left + bounds.Width() / 2 - stringWidth / 2,
			bounds.bottom - bounds.Height() / 2 + stringHeight / 2));
}


void
Barberpole::_DrawOnBitmap()
{
	if (!fBitmap->Lock())
		return;

	BRect rect = fBitmap->Bounds();

	fBitmapView->SetDrawingMode(B_OP_COPY);

	rgb_color color = ui_color(B_STATUS_BAR_COLOR);
	fBitmapView->SetHighColor(color);

	// draw the pole
	rect.InsetBy(2, 2);
	fBitmapView->FillRect(rect, fPattern);

	// draw frame -- left (darker)
	color.red = 150;
	color.green = 150;
	color.blue = 150;
	fBitmapView->SetHighColor(color);
	fBitmapView->SetDrawingMode(B_OP_OVER);

	BPoint pointA = fBitmap->Bounds().LeftTop();
	BPoint pointB = fBitmap->Bounds().LeftBottom();
	pointB.y -= 1;
	fBitmapView->StrokeLine(pointA, pointB);
	pointA.x += 1;
	pointB.x += 1;
	pointB.y -= 1;
	fBitmapView->StrokeLine(pointA, pointB);

	// top
	pointA = fBitmap->Bounds().LeftTop();
	pointB = fBitmap->Bounds().RightTop();
	pointB.x -= 1;
	fBitmapView->StrokeLine(pointA, pointB);
	pointA.y += 1;
	pointB.y += 1;
	pointB.x -= 1;
	fBitmapView->StrokeLine(pointA, pointB);

	// right (lighter)
	color.red = 255;
	color.green = 255;
	color.blue = 255;
	fBitmapView->SetHighColor(color);
	pointA = fBitmap->Bounds().RightTop();
	pointB = fBitmap->Bounds().RightBottom();
	fBitmapView->StrokeLine(pointA, pointB);
	pointA.y += 1;
	pointA.x -= 1;
	pointB.x -= 1;
	fBitmapView->StrokeLine(pointA, pointB);

	// bottom
	pointA = fBitmap->Bounds().LeftBottom();
	pointB = fBitmap->Bounds().RightBottom();
	fBitmapView->StrokeLine(pointA, pointB);
	pointA.x += 1;
	pointA.y -= 1;
	pointB.y -= 1;
	fBitmapView->StrokeLine(pointA, pointB);

	// bevel blending
	color.red = 150;
	color.green = 150;
	color.blue = 150;
	fBitmapView->SetHighColor(color);
	fBitmapView->SetDrawingMode(B_OP_SUBTRACT);
	fBitmapView->StrokeRect(rect);

	rect.InsetBy(1, 1);
	_LightenBitmapHighColor(&color);
	fBitmapView->StrokeRect(rect);

	rect.InsetBy(1, 1);
	_LightenBitmapHighColor(&color);
	fBitmapView->StrokeRect(rect);

	rect.InsetBy(1, 1);
	_LightenBitmapHighColor(&color);
	fBitmapView->StrokeRect(rect);

	rect.InsetBy(1, 1);
	_LightenBitmapHighColor(&color);
	fBitmapView->StrokeRect(rect);

	fBitmapView->Sync();
	fBitmap->Unlock();
}


void
Barberpole::_LightenBitmapHighColor(rgb_color* color)
{
	color->red -= 30;
	color->green -= 30;
	color->blue -= 30;

	fBitmapView->SetHighColor(*color);
}


void
Barberpole::_CreateBitmap()
{
	BRect rect = Bounds();
	fBitmap = new BBitmap(rect, B_CMAP8, true);
	fBitmapView = new BView(rect, "buffer", B_FOLLOW_NONE, 0);
	fBitmap->AddChild(fBitmapView);
}


void
Barberpole::FrameMoved(BPoint point)
{
	Invalidate();
}


void
Barberpole::FrameResized(float width, float height)
{
	delete fBitmap;
	_CreateBitmap();
	Invalidate();
}
