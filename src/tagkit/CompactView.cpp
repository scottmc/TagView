/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
#include "CompactView.h"

#include <math.h>
#include <stdio.h>

#include <vector>

#include <Bitmap.h>
#include <Font.h>
#include <GradientLinear.h>
#include <InterfaceDefs.h>
#include <Rect.h>
#include <Size.h>
#include <String.h>

#include "TagRecord.h"


namespace tagkit {

namespace {

const float kOuterMargin = 4.0f;		// view edge to the outline
const float kOuterPadding = 10.0f;		// outline to the squares
const float kSquareGap = 10.0f;			// between the two squares
const float kTextPadding = 8.0f;		// inside the info square
const float kMinFontSize = 9.0f;
const float kMaxFontSize = 28.0f;

// Smallest the view will be asked to shrink to.
const float kMinWidth = 300.0f;
const float kMinHeight = 150.0f;


struct InfoRow {
	BString	label;
	BString	value;
};


BString
FormatDuration(int32 seconds)
{
	if (seconds < 0)
		return BString();

	char buffer[16];
	snprintf(buffer, sizeof(buffer), "%d:%02d", (int)(seconds / 60),
		(int)(seconds % 60));
	return BString(buffer);
}


BString
FormatPositive(int32 number)
{
	// Zero means "unknown" in a TagRecord; show it as blank.
	BString text;
	if (number > 0)
		text << number;
	return text;
}

} // namespace


// #pragma mark - CompactInfoSquare


class CompactInfoSquare : public BView {
public:
	CompactInfoSquare()
		:
		BView("compactInfoSquare", B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE)
	{
		SetViewColor(B_TRANSPARENT_COLOR);
	}

	void SetRecord(const TagRecord* record)
	{
		fRows.clear();

		if (record != NULL) {
			_AddRow("Artist:", record->artist);
			_AddRow("Title:", record->title);
			_AddRow("Album:", record->album);
			_AddRow("Track:", FormatPositive(record->track));
			_AddRow("Year:", FormatPositive(record->year));
			_AddRow("Genre:", record->genre);
			_AddRow("Duration:", FormatDuration(record->durationSeconds));
			_AddRow("Format:", BString(format_label(record->format)));
		}

		Invalidate();
	}

	virtual void Draw(BRect updateRect)
	{
		BRect bounds = Bounds();

		SetHighColor(ui_color(B_DOCUMENT_BACKGROUND_COLOR));
		FillRect(bounds);
		SetHighColor(ui_color(B_CONTROL_BORDER_COLOR));
		StrokeRect(bounds);

		SetLowColor(ui_color(B_DOCUMENT_BACKGROUND_COLOR));
		SetHighColor(ui_color(B_DOCUMENT_TEXT_COLOR));

		if (fRows.empty()) {
			_DrawPlaceholder(bounds);
			return;
		}

		// Scale the text with the square: pick the size that makes all the
		// rows fit the height, within sane limits.
		float rowCount = (float)fRows.size();
		float available = bounds.Height() + 1.0f - (2 * kTextPadding);
		float size = available / (rowCount * 1.35f);
		if (size < kMinFontSize)
			size = kMinFontSize;
		if (size > kMaxFontSize)
			size = kMaxFontSize;

		BFont plain(be_plain_font);
		plain.SetSize(size);
		BFont bold(be_bold_font);
		bold.SetSize(size);

		font_height fontHeight;
		plain.GetHeight(&fontHeight);
		float lineHeight = ceilf(fontHeight.ascent + fontHeight.descent
			+ fontHeight.leading);
		float step = available / rowCount;
		if (step > lineHeight * 1.6f)
			step = lineHeight * 1.6f;

		float textWidth = bounds.Width() + 1.0f - (2 * kTextPadding);
		float y = bounds.top + kTextPadding + fontHeight.ascent;

		for (size_t i = 0; i < fRows.size(); i++) {
			const InfoRow& row = fRows[i];

			SetFont(&bold);
			DrawString(row.label.String(),
				BPoint(bounds.left + kTextPadding, y));

			float labelWidth = bold.StringWidth(row.label.String())
				+ bold.StringWidth(" ");
			BString value(row.value);
			plain.TruncateString(&value, B_TRUNCATE_END,
				textWidth - labelWidth);

			SetFont(&plain);
			DrawString(value.String(),
				BPoint(bounds.left + kTextPadding + labelWidth, y));

			y += step;
		}
	}

private:
	void _AddRow(const char* label, const BString& value)
	{
		InfoRow row;
		row.label = label;
		row.value = value;
		fRows.push_back(row);
	}

	void _DrawPlaceholder(BRect bounds)
	{
		const char* label = "No file selected";
		BFont font(be_plain_font);
		SetFont(&font);
		SetHighColor(tint_color(ui_color(B_DOCUMENT_TEXT_COLOR),
			B_LIGHTEN_1_TINT));

		font_height fontHeight;
		font.GetHeight(&fontHeight);
		DrawString(label, BPoint(
			bounds.left + (bounds.Width() - StringWidth(label)) / 2.0f,
			bounds.top + (bounds.Height() + fontHeight.ascent
				- fontHeight.descent) / 2.0f));
	}

	std::vector<InfoRow>	fRows;
};


// #pragma mark - CompactArtSquare


class CompactArtSquare : public BView {
public:
	CompactArtSquare()
		:
		BView("compactArtSquare", B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
		fBitmap(NULL)
	{
		SetViewColor(B_TRANSPARENT_COLOR);
	}

	virtual ~CompactArtSquare()
	{
		delete fBitmap;
	}

	void SetBitmap(BBitmap* bitmap)
	{
		if (bitmap != fBitmap) {
			delete fBitmap;
			fBitmap = bitmap;
		}
		Invalidate();
	}

	virtual void Draw(BRect updateRect)
	{
		BRect bounds = Bounds();

		if (fBitmap != NULL) {
			// Fit inside the square keeping the image's aspect ratio,
			// centered (covers are normally square already).
			SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
			FillRect(bounds);

			BRect source = fBitmap->Bounds();
			float scale = bounds.Width() / (source.Width() + 1.0f);
			float heightScale = bounds.Height() / (source.Height() + 1.0f);
			if (heightScale < scale)
				scale = heightScale;

			float width = (source.Width() + 1.0f) * scale;
			float height = (source.Height() + 1.0f) * scale;
			BRect target(0, 0, width - 1.0f, height - 1.0f);
			target.OffsetBy(
				bounds.left + (bounds.Width() + 1.0f - width) / 2.0f,
				bounds.top + (bounds.Height() + 1.0f - height) / 2.0f);

			SetDrawingMode(B_OP_ALPHA);
			SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
			DrawBitmap(fBitmap, source, target, B_FILTER_BITMAP_BILINEAR);
			SetDrawingMode(B_OP_COPY);
		} else {
			// No art: a blue gradient, light at the top to deep at the
			// bottom.
			BGradientLinear gradient(bounds.LeftTop(), bounds.LeftBottom());
			gradient.AddColor(make_color(120, 175, 245), 0);
			gradient.AddColor(make_color(25, 70, 160), 255);
			FillRect(bounds, gradient);
		}

		SetHighColor(ui_color(B_CONTROL_BORDER_COLOR));
		StrokeRect(bounds);
	}

private:
	BBitmap*	fBitmap;
};


// #pragma mark - CompactView


CompactView::CompactView(const char* name)
	:
	BView(name, B_WILL_DRAW | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE),
	fInfoSquare(new CompactInfoSquare()),
	fArtSquare(new CompactArtSquare())
{
	SetViewColor(B_TRANSPARENT_COLOR);

	AddChild(fInfoSquare);
	AddChild(fArtSquare);

	SetExplicitMinSize(BSize(kMinWidth, kMinHeight));
}


CompactView::~CompactView()
{
}


void
CompactView::AttachedToWindow()
{
	BView::AttachedToWindow();
	_LayoutSquares();
}


void
CompactView::FrameResized(float width, float height)
{
	BView::FrameResized(width, height);
	_LayoutSquares();
}


void
CompactView::Draw(BRect updateRect)
{
	BRect bounds = Bounds();

	SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	FillRect(bounds);

	SetHighColor(ui_color(B_CONTROL_BORDER_COLOR));
	StrokeRect(bounds.InsetByCopy(kOuterMargin, kOuterMargin));
}


void
CompactView::SetRecord(const TagRecord* record)
{
	fInfoSquare->SetRecord(record);
}


void
CompactView::SetCoverBitmap(BBitmap* bitmap)
{
	fArtSquare->SetBitmap(bitmap);
}


void
CompactView::_LayoutSquares()
{
	// Two equal squares side by side, as big as fit inside the outline
	// (limited by its height or by half its width), centered as a pair.
	BRect inner = Bounds().InsetByCopy(kOuterMargin + kOuterPadding,
		kOuterMargin + kOuterPadding);
	float innerWidth = inner.Width() + 1.0f;
	float innerHeight = inner.Height() + 1.0f;

	float side = floorf(fminf(innerHeight, (innerWidth - kSquareGap) / 2.0f));
	if (side < 1.0f)
		side = 1.0f;

	float totalWidth = (2 * side) + kSquareGap;
	float left = floorf(inner.left + (innerWidth - totalWidth) / 2.0f);
	float top = floorf(inner.top + (innerHeight - side) / 2.0f);

	fInfoSquare->MoveTo(left, top);
	fInfoSquare->ResizeTo(side - 1.0f, side - 1.0f);
	fArtSquare->MoveTo(left + side + kSquareGap, top);
	fArtSquare->ResizeTo(side - 1.0f, side - 1.0f);
}


} // namespace tagkit
