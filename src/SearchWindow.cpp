/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
#include "SearchWindow.h"

#include <Autolock.h>
#include <Button.h>
#include <LayoutBuilder.h>
#include <Message.h>
#include <TextControl.h>

#include <stdio.h>
#include <stdlib.h>

#include "Messages.h"
#include "Settings.h"


static const float kWindowWidth = 320;


// Parses a track time typed as "m:ss" (or "h:mm:ss"), or as plain
// seconds. Returns false for anything else (negative numbers, seconds
// >= 60 after a colon, stray characters, ...). Blank text is not a valid
// time here -- callers check for that first, since blank just means "no
// time given".
static bool
parse_track_time(const char* text, int32& seconds)
{
	if (text == NULL)
		return false;

	int64 total = 0;
	int fields = 0;
	const char* p = text;

	while (true) {
		while (*p == ' ')
			p++;
		if (*p < '0' || *p > '9')
			return false;

		int64 value = 0;
		while (*p >= '0' && *p <= '9') {
			value = value * 10 + (*p - '0');
			if (value > 100000)
				return false;
			p++;
		}
		while (*p == ' ')
			p++;

		// Fields after the first (minutes/seconds) must be 0-59.
		if (fields > 0 && value >= 60)
			return false;

		total = total * 60 + value;
		fields++;

		if (*p == ':') {
			if (fields >= 3)
				return false;
			p++;
			continue;
		}
		break;
	}

	if (*p != '\0')
		return false;

	seconds = (int32)total;
	return true;
}


static BString
format_track_time(int32 seconds)
{
	char buffer[16];
	snprintf(buffer, sizeof(buffer), "%d:%02d", (int)(seconds / 60),
		(int)(seconds % 60));
	return BString(buffer);
}


SearchWindow::SearchWindow(BMessenger target)
	:
	BWindow(BRect(120, 120, 120 + kWindowWidth, 250), "Search MusicBrainz",
		B_TITLED_WINDOW,
		B_NOT_ZOOMABLE | B_NOT_RESIZABLE | B_AUTO_UPDATE_SIZE_LIMITS
			| B_ASYNCHRONOUS_CONTROLS),
	fTarget(target)
{
	fArtistControl = new BTextControl("artist", "Artist:", "", NULL);
	fArtistControl->SetModificationMessage(
		new BMessage(kMsgSearchTextChanged));

	fSongControl = new BTextControl("song", "Song:", "", NULL);
	fSongControl->SetModificationMessage(new BMessage(kMsgSearchTextChanged));
	fSongControl->SetToolTip("Leave blank to search for cover art only, "
		"using the Artist and Album.");

	fAlbumControl = new BTextControl("album", "Album:", "", NULL);
	fAlbumControl->SetToolTip("Optional. Narrows the search to recordings on "
		"a release with this title.");

	fTimeControl = new BTextControl("time", "Time:", "", NULL);
	fTimeControl->SetModificationMessage(new BMessage(kMsgSearchTextChanged));
	fTimeControl->SetToolTip("Optional track length as m:ss. Results whose "
		"length is closest to this are listed first.");

	fSearchButton = new BButton("search", "Search",
		new BMessage(kMsgSearchRequested));
	fSearchButton->SetEnabled(false);

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_WINDOW_INSETS)
		.Add(fArtistControl)
		.Add(fSongControl)
		.Add(fTimeControl)
		.Add(fAlbumControl)
		.AddGlue()
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(fSearchButton)
		.End()
		.End();

	fArtistControl->SetTarget(this);
	fSongControl->SetTarget(this);
	fAlbumControl->SetTarget(this);
	fTimeControl->SetTarget(this);
	fSearchButton->SetTarget(this);

	fArtistControl->MakeFocus(true);

	Settings::Get().RestoreWindow("search", this);
}


void
SearchWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgSearchTextChanged:
			_UpdateSearchButtonEnabled();
			break;

		case kMsgSearchRequested:
		{
			BMessage request(kMsgSearchRequested);
			request.AddString("artist", fArtistControl->Text());
			request.AddString("song", fSongControl->Text());

			if (fAlbumControl->TextLength() > 0)
				request.AddString("album", fAlbumControl->Text());

			int32 seconds;
			if (fTimeControl->TextLength() > 0
					&& parse_track_time(fTimeControl->Text(), seconds))
				request.AddInt32("durationSeconds", seconds);
			fTarget.SendMessage(&request);

			// The search is under way (and its progress shows in the main
			// window), so the dialog has done its job. QuitRequested()
			// lets TagViewWindow know it's gone.
			PostMessage(B_QUIT_REQUESTED);
			break;
		}

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


bool
SearchWindow::QuitRequested()
{
	// Let TagViewWindow know we're going away so it can drop its pointer
	// to us instead of holding on to a stale one.
	Settings::Get().SaveWindow("search", this);
	fTarget.SendMessage(kMsgSearchWindowClosed);
	return true;
}


void
SearchWindow::SetQuery(const char* artist, const char* song,
	const char* album, int32 durationSeconds)
{
	// Called from TagViewWindow's thread, not this window's own, so the
	// window has to be locked before touching its views -- otherwise the
	// app drops into the debugger ("Looper must be locked") whenever
	// Edit > Search... is chosen while this window already exists.
	BAutolock locker(this);
	if (!locker.IsLocked())
		return;

	fArtistControl->SetText(artist != NULL ? artist : "");
	fSongControl->SetText(song != NULL ? song : "");
	fAlbumControl->SetText(album != NULL ? album : "");
	fTimeControl->SetText(durationSeconds >= 0
		? format_track_time(durationSeconds).String() : "");
	_UpdateSearchButtonEnabled();
}


void
SearchWindow::_UpdateSearchButtonEnabled()
{
	// Artist is required. Song is too for a recording search; without one
	// the search is for cover art only (Time doesn't apply then). Time is
	// optional, but if something is typed there with a song it has to be a
	// time we can understand.
	bool ready = fArtistControl->TextLength() > 0;

	if (ready && fSongControl->TextLength() > 0
			&& fTimeControl->TextLength() > 0) {
		int32 seconds;
		ready = parse_track_time(fTimeControl->Text(), seconds);
	}
	fSearchButton->SetEnabled(ready);
}


void
SearchWindow::MoveToFrontAndFocus()
{
	if (IsHidden())
		Show();
	Activate();
}
