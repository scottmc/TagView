/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
#include "CoverArtPickerWindow.h"

#include <Autolock.h>
#include <Bitmap.h>
#include <Button.h>
#include <LayoutBuilder.h>
#include <Message.h>
#include <ScrollView.h>
#include <StringView.h>

#include "Messages.h"
#include "Settings.h"
#include "tagkit/CoverArtCandidatesView.h"

using tagkit::CoverArtCandidatesView;
using tagkit::CoverArtImage;


static const float kWindowWidth = 480;
static const float kWindowHeight = 250;

// Thumbnails visible at once in the picker; more covers scroll.
static const int32 kImagesWide = 4;


CoverArtPickerWindow::CoverArtPickerWindow(BMessenger target,
	const BString& fileName, const std::vector<CoverArtImage>& images)
	:
	BWindow(BRect(180, 180, 180 + kWindowWidth, 180 + kWindowHeight),
		"Choose Cover Art", B_TITLED_WINDOW,
			B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS),
	fTarget(target)
{
	fPromptText << "Cover art for \"" << fileName << "\"";
	fPromptView = new BStringView("prompt", "");
	fPromptView->SetText(fPromptText.String());

	fCandidatesView = new CoverArtCandidatesView(kImagesWide);
	fCandidatesView->SetSelectionMessage(
		new BMessage(kMsgCoverArtSelectionChanged));
	fCandidatesView->SetInvocationMessage(new BMessage(kMsgCoverArtChosen));

	// Decode every candidate; the view takes ownership of the bitmaps. A
	// candidate that won't decode stays in the list as a placeholder so
	// indexes keep matching the caller's vector.
	BBitmap** bitmaps = new BBitmap*[images.size()];
	for (size_t i = 0; i < images.size(); i++) {
		bitmaps[i] = tagkit::decode_cover_art(images[i]);

		BString caption(images[i].releaseTitle);
		if (images[i].year > 0)
			caption << " (" << images[i].year << ")";
		fCaptions.push_back(caption);
	}
	fCandidatesView->SetCandidates(bitmaps, (int32)images.size());
	delete[] bitmaps;

	BScrollView* scrollView = new BScrollView("candidatesScroll",
		fCandidatesView, B_WILL_DRAW | B_FRAME_EVENTS, true /* horizontal */,
		false /* vertical */, B_FANCY_BORDER);

	fCaptionView = new BStringView("caption", "");

	BButton* cancelButton = new BButton("cancel", "Cancel",
		new BMessage(B_QUIT_REQUESTED));
	BButton* useButton = new BButton("use", "Use Selected Cover",
		new BMessage(kMsgCoverArtChosen));
	useButton->MakeDefault(true);

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_WINDOW_INSETS)
		.Add(fPromptView)
		.Add(scrollView)
		.Add(fCaptionView)
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(cancelButton)
			.Add(useButton)
		.End()
		.End();

	cancelButton->SetTarget(this);
	useButton->SetTarget(this);

	_UpdateCaption();

	// Fit the four-thumbnail strip rather than the guessed size above.
	ResizeToPreferred();

	Settings::Get().RestoreWindow("picker", this);
}


void
CoverArtPickerWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgCoverArtSelectionChanged:
			_UpdateCaption();
			break;

		case kMsgCoverArtChosen:
			_Choose();
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


bool
CoverArtPickerWindow::QuitRequested()
{
	// Let TagViewWindow drop its pointer to us instead of holding a stale
	// one.
	Settings::Get().SaveWindow("picker", this);
	fTarget.SendMessage(kMsgCoverArtWindowClosed);
	return true;
}


void
CoverArtPickerWindow::AddImage(const CoverArtImage& image)
{
	// Called from TagViewWindow's thread, so lock this window first.
	BAutolock locker(this);
	if (!locker.IsLocked())
		return;

	BString caption(image.releaseTitle);
	if (image.year > 0)
		caption << " (" << image.year << ")";
	fCaptions.push_back(caption);

	fCandidatesView->AddCandidate(tagkit::decode_cover_art(image));
}


void
CoverArtPickerWindow::SetLoading(bool loading)
{
	BAutolock locker(this);
	if (!locker.IsLocked())
		return;

	BString text(fPromptText);
	text << (loading ? " (still looking" B_UTF8_ELLIPSIS ")" : ":");
	fPromptView->SetText(text.String());
}


void
CoverArtPickerWindow::MoveToFrontAndFocus()
{
	if (IsHidden())
		Show();
	Activate();
}


void
CoverArtPickerWindow::_UpdateCaption()
{
	int32 index = fCandidatesView->SelectedIndex();
	if (index >= 0 && index < (int32)fCaptions.size())
		fCaptionView->SetText(fCaptions[index].String());
	else
		fCaptionView->SetText("");
}


void
CoverArtPickerWindow::_Choose()
{
	int32 index = fCandidatesView->SelectedIndex();
	if (index < 0 || index >= fCandidatesView->CountCandidates())
		return;

	BMessage chosen(kMsgCoverArtChosen);
	chosen.AddInt32("index", index);
	fTarget.SendMessage(&chosen);

	PostMessage(B_QUIT_REQUESTED);
}
