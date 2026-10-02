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
#include <Message.h>
#include <GradientLinear.h>
#include <InterfaceDefs.h>
#include <Rect.h>
#include <Size.h>
#include <String.h>
#include <Window.h>

#include "TagField.h"
#include "TagRecord.h"


namespace tagkit {

namespace {

const float kOuterMargin = 4.0f;		// view edge to the outline
const float kOuterPadding = 10.0f;		// outline to the squares
const float kSquareGap = 10.0f;			// between the two squares
const float kFileNameGap = 8.0f;		// squares to the file name
const float kTextPadding = 8.0f;		// inside the info square
const float kMinFontSize = 9.0f;
const float kMaxFontSize = 28.0f;
const float kMinTextScale = 0.5f;
const float kMaxTextScale = 2.0f;

// Smallest the view will be asked to shrink to.
const float kMinWidth = 300.0f;
const float kMinHeight = 150.0f;


struct InfoRow {
	BString	label;
	BString	value;
	int32	field;		// a tag_field if the row can be edited, else -1
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


// Splits text so that first fits in width: at the last space that fits, or
// failing that mid-word. rest gets whatever is left over (empty if it all
// fit).
void
WrapOnce(const BFont& font, const BString& text, float width, BString& first,
	BString& rest)
{
	first = text;
	rest = "";
	if (font.StringWidth(text.String()) <= width)
		return;

	int32 length = text.Length();
	int32 lastSpace = -1;	// last space whose prefix still fits
	int32 lastFit = 0;		// end of the longest prefix that fits
	for (int32 i = 0; i < length;) {
		int32 next = i + 1;
		while (next < length && (text.ByteAt(next) & 0xC0) == 0x80)
			next++;

		BString prefix;
		text.CopyInto(prefix, 0, next);
		if (font.StringWidth(prefix.String()) > width)
			break;
		lastFit = next;
		if (text.ByteAt(i) == ' ')
			lastSpace = i;
		i = next;
	}

	int32 breakAt = lastSpace > 0 ? lastSpace : lastFit;
	if (breakAt <= 0)
		breakAt = length > 0 ? 1 : 0;

	text.CopyInto(first, 0, breakAt);
	text.CopyInto(rest, breakAt, length - breakAt);
	rest.Trim();
}

} // namespace


// #pragma mark - CompactInfoSquare


class CompactInfoSquare : public BView {
public:
	CompactInfoSquare(CompactView* owner)
		:
		BView("compactInfoSquare", B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
		fOwner(owner),
		fTextScale(1.0f)
	{
		SetViewColor(B_TRANSPARENT_COLOR);
	}

	void SetTextScale(float scale)
	{
		if (scale != fTextScale) {
			fTextScale = scale;
			Invalidate();
		}
	}

	void SetRecord(const TagRecord* record)
	{
		fRows.clear();

		if (record != NULL) {
			_AddRow("Artist:", record->artist, TAG_FIELD_ARTIST);
			_AddRow("Title:", record->title, TAG_FIELD_TITLE);
			_AddRow("Album:", record->album, TAG_FIELD_ALBUM);
			_AddRow("Track:", FormatPositive(record->track), TAG_FIELD_TRACK);
			_AddRow("Year:", FormatPositive(record->year), TAG_FIELD_YEAR);
			_AddRow("Genre:", record->genre, TAG_FIELD_GENRE);
			_AddRow("Duration:", FormatDuration(record->durationSeconds), -1);
			_AddRow("Format:", BString(format_label(record->format)), -1);
		}

		Invalidate();
	}

	virtual void MouseDown(BPoint where)
	{
		// A right-click on an editable row asks to edit that field.
		BMessage* current = Window() != NULL ? Window()->CurrentMessage()
			: NULL;
		int32 buttons = 0;
		if (current == NULL || current->FindInt32("buttons", &buttons) != B_OK
				|| (buttons & B_SECONDARY_MOUSE_BUTTON) == 0) {
			return;
		}

		Layout layout;
		if (!_Layout(layout))
			return;

		// Rows with a wrapped second line own both lines.
		float y = where.y - Bounds().top - kTextPadding;
		int32 index = -1;
		float top = 0;
		for (size_t i = 0; i < layout.rows.size(); i++) {
			float height = layout.step * (layout.rows[i].rest.Length() > 0
				? 2 : 1);
			if (y >= top && y < top + height) {
				index = (int32)i;
				break;
			}
			top += height;
		}
		if (index < 0 || fRows[index].field < 0)
			return;

		fOwner->_RequestEdit(fRows[index].field, ConvertToScreen(where),
			Bounds().Width() + 1.0f);
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

		Layout layout;
		if (!_Layout(layout)) {
			_DrawPlaceholder(bounds);
			return;
		}

		BFont plain(be_plain_font);
		plain.SetSize(layout.size);
		BFont bold(be_bold_font);
		bold.SetSize(layout.size);

		float textWidth = bounds.Width() + 1.0f - (2 * kTextPadding);
		float y = bounds.top + kTextPadding + layout.fontHeight.ascent;

		for (size_t i = 0; i < fRows.size(); i++) {
			const InfoRow& row = fRows[i];
			const LaidOutRow& laidOut = layout.rows[i];

			SetFont(&bold);
			DrawString(row.label.String(),
				BPoint(bounds.left + kTextPadding, y));

			float valueLeft = bounds.left + kTextPadding + laidOut.labelWidth;
			SetFont(&plain);
			DrawString(laidOut.first.String(), BPoint(valueLeft, y));
			y += layout.step;

			if (laidOut.rest.Length() > 0) {
				// The one extra line; still ends in an ellipsis if cut.
				BString rest(laidOut.rest);
				plain.TruncateString(&rest, B_TRUNCATE_END,
					textWidth - laidOut.labelWidth);
				DrawString(rest.String(), BPoint(valueLeft, y));
				y += layout.step;
			}
		}
	}

private:
	void _AddRow(const char* label, const BString& value, int32 field)
	{
		InfoRow row;
		row.label = label;
		row.value = value;
		row.field = field;
		fRows.push_back(row);
	}

	struct LaidOutRow {
		BString	first;			// value text on the row's own line
		BString	rest;			// the wrapped second line, if any
		float	labelWidth;
	};

	struct Layout {
		float					size;
		float					step;		// from one line to the next
		font_height				fontHeight;
		std::vector<LaidOutRow>	rows;
	};

	// Works out how the rows are laid out for the square's current size:
	// the font size (scaled with the square and by the text scale, within
	// limits), which rows wrap onto a second line (at most one extra line
	// each), and the distance between lines. If the wrapped rows don't fit
	// the height the font shrinks until they do (or hits its minimum).
	// Drawing and hit-testing both use it so they always agree. False if
	// there are no rows.
	bool _Layout(Layout& layout) const
	{
		if (fRows.empty())
			return false;

		BRect bounds = Bounds();
		float available = bounds.Height() + 1.0f - (2 * kTextPadding);
		float textWidth = bounds.Width() + 1.0f - (2 * kTextPadding);

		float size = available / ((float)fRows.size() * 1.35f) * fTextScale;

		for (int attempt = 0; attempt < 8; attempt++) {
			if (size < kMinFontSize)
				size = kMinFontSize;
			if (size > kMaxFontSize)
				size = kMaxFontSize;

			BFont plain(be_plain_font);
			plain.SetSize(size);
			BFont bold(be_bold_font);
			bold.SetSize(size);
			plain.GetHeight(&layout.fontHeight);
			float lineHeight = ceilf(layout.fontHeight.ascent
				+ layout.fontHeight.descent + layout.fontHeight.leading);

			layout.size = size;
			layout.rows.clear();
			int32 lines = 0;
			for (size_t i = 0; i < fRows.size(); i++) {
				LaidOutRow row;
				row.labelWidth = bold.StringWidth(fRows[i].label.String())
					+ bold.StringWidth(" ");
				WrapOnce(plain, fRows[i].value,
					textWidth - row.labelWidth, row.first, row.rest);
				lines += row.rest.Length() > 0 ? 2 : 1;
				layout.rows.push_back(row);
			}

			layout.step = available / (float)lines;
			if (layout.step > lineHeight * 1.6f)
				layout.step = lineHeight * 1.6f;

			if (layout.step >= lineHeight * 1.05f || size <= kMinFontSize)
				break;
			// Too tight: shrink in proportion and try again.
			size *= (layout.step / (lineHeight * 1.05f)) * 0.98f;
		}
		return true;
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

	CompactView*			fOwner;
	std::vector<InfoRow>	fRows;
	float					fTextScale;
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
	fInfoSquare(new CompactInfoSquare(this)),
	fArtSquare(new CompactArtSquare()),
	fEditMessage(NULL),
	fTextScale(1.0f)
{
	SetViewColor(B_TRANSPARENT_COLOR);

	AddChild(fInfoSquare);
	AddChild(fArtSquare);

	SetExplicitMinSize(BSize(kMinWidth, kMinHeight));
}


CompactView::~CompactView()
{
	delete fEditMessage;
}


void
CompactView::SetTextScale(float scale)
{
	if (scale < kMinTextScale)
		scale = kMinTextScale;
	if (scale > kMaxTextScale)
		scale = kMaxTextScale;
	if (scale == fTextScale)
		return;

	fTextScale = scale;
	fInfoSquare->SetTextScale(scale);
}


void
CompactView::SetEditMessage(BMessage* message)
{
	delete fEditMessage;
	fEditMessage = message;
}


void
CompactView::_RequestEdit(int32 field, BPoint screenWhere, float width)
{
	if (fEditMessage == NULL || Window() == NULL)
		return;

	BMessage* request = new BMessage(*fEditMessage);
	request->AddInt32("field", field);
	request->AddPoint("where", screenWhere);
	request->AddFloat("width", width);
	Window()->PostMessage(request);
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


float
CompactView::_FileNameStripHeight() const
{
	BFont font(be_plain_font);
	font_height fontHeight;
	font.GetHeight(&fontHeight);
	return ceilf(fontHeight.ascent + fontHeight.descent
		+ fontHeight.leading) + kFileNameGap;
}


void
CompactView::Draw(BRect updateRect)
{
	BRect bounds = Bounds();

	SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	FillRect(bounds);

	SetHighColor(ui_color(B_CONTROL_BORDER_COLOR));
	StrokeRect(bounds.InsetByCopy(kOuterMargin, kOuterMargin));

	// The file name sits under the two squares, inside the outline,
	// centered and shortened (in the middle, so the extension stays) to
	// fit the width of the pair.
	if (fFileName.Length() > 0) {
		BFont font(be_plain_font);
		font_height fontHeight;
		font.GetHeight(&fontHeight);

		BRect squares = fInfoSquare->Frame() | fArtSquare->Frame();
		BString text(fFileName);
		font.TruncateString(&text, B_TRUNCATE_MIDDLE, squares.Width() + 1.0f);

		SetFont(&font);
		SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		SetHighColor(ui_color(B_PANEL_TEXT_COLOR));
		DrawString(text.String(), BPoint(
			squares.left + (squares.Width() + 1.0f - font.StringWidth(
				text.String())) / 2.0f,
			squares.bottom + kFileNameGap + fontHeight.ascent));
	}
}


void
CompactView::SetRecord(const TagRecord* record)
{
	fInfoSquare->SetRecord(record);

	fFileName = record != NULL ? record->fileName : BString();
	Invalidate();
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

	// Leave a strip under the squares for the file name.
	innerHeight -= _FileNameStripHeight();

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
	Invalidate();
}


} // namespace tagkit
