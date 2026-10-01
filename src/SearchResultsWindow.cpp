/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
#include "SearchResultsWindow.h"

#include <Autolock.h>
#include <Button.h>
#include <LayoutBuilder.h>
#include <Message.h>
#include <stdio.h>

#include "Messages.h"
#include "tagkit/RecordingMatchView.h"

using tagkit::RecordingMatch;
using tagkit::RecordingMatchView;


static const float kWindowWidth = 620;
static const float kWindowHeight = 260;


SearchResultsWindow::SearchResultsWindow(BMessenger target,
	const BString& artist, const BString& song, int32 durationSeconds)
	:
	BWindow(BRect(160, 160, 160 + kWindowWidth, 160 + kWindowHeight),
		"MusicBrainz Results", B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS),
	fTarget(target)
{
	SetQuery(artist, song, durationSeconds);

	fMatchView = new RecordingMatchView("matchView");
	fMatchView->SetInvocationMessage(new BMessage(kMsgApplyMatch));

	fApplyButton = new BButton("apply", "Apply to Selected File",
		new BMessage(kMsgApplyMatch));
	fApplyButton->SetEnabled(false);

	BButton* cancelButton = new BButton("cancel", "Cancel",
		new BMessage(B_QUIT_REQUESTED));

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_WINDOW_INSETS)
		.Add(fMatchView)
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(cancelButton)
			.Add(fApplyButton)
		.End()
		.End();

	fApplyButton->SetTarget(this);
	cancelButton->SetTarget(this);
}


void
SearchResultsWindow::SetQuery(const BString& artist, const BString& song,
	int32 durationSeconds)
{
	// Callable from another window's thread (TagViewWindow's), so lock
	// this window first. See SearchWindow::SetQuery().
	BAutolock locker(this);
	if (!locker.IsLocked())
		return;

	BString title("Results for \"");
	title << artist << "\" - \"" << song << "\"";
	if (durationSeconds >= 0) {
		char time[16];
		snprintf(time, sizeof(time), " - %d:%02d", (int)(durationSeconds / 60),
			(int)(durationSeconds % 60));
		title << time;
	}
	SetTitle(title.String());
}


void
SearchResultsWindow::SetMatches(const std::vector<RecordingMatch>& matches)
{
	// Same as SetQuery(): called from TagViewWindow's thread.
	BAutolock locker(this);
	if (!locker.IsLocked())
		return;

	fMatchView->Clear();
	for (size_t i = 0; i < matches.size(); i++)
		fMatchView->AddMatch(matches[i]);

	fApplyButton->SetEnabled(matches.size() > 0);
}


void
SearchResultsWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgApplyMatch:
			_Apply();
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void
SearchResultsWindow::_Apply()
{
	const RecordingMatch* match = fMatchView->SelectedMatch();
	if (match == NULL)
		return;

	BMessage result(kMsgApplyMatch);
	result.AddString("id", match->id);
	result.AddString("title", match->title);
	result.AddString("artist", match->artist);
	result.AddString("album", match->album);
	result.AddInt32("durationSeconds", match->durationSeconds);
	result.AddInt32("track", match->track);
	result.AddInt32("year", match->year);

	// Parallel arrays: every release the recording is on, for the cover
	// art lookup that follows.
	for (size_t i = 0; i < match->releases.size(); i++) {
		result.AddString("releaseId", match->releases[i].id);
		result.AddString("releaseTitle", match->releases[i].title);
		result.AddInt32("releaseYear", match->releases[i].year);
	}
	fTarget.SendMessage(&result);

	PostMessage(B_QUIT_REQUESTED);
}


bool
SearchResultsWindow::QuitRequested()
{
	fTarget.SendMessage(kMsgResultsWindowClosed);
	return true;
}


void
SearchResultsWindow::MoveToFrontAndFocus()
{
	if (IsHidden())
		Show();
	Activate();
}
