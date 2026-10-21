/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
#include "ReleaseLookupWindow.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Button.h>
#include <Font.h>
#include <LayoutBuilder.h>
#include <Message.h>
#include <Messenger.h>
#include <OS.h>
#include <ScrollView.h>
#include <String.h>
#include <TextControl.h>
#include <TextView.h>

#include "tagkit/MusicBrainzSearch.h"
#include "tagkit/ReleaseInfo.h"


namespace {

const uint32 kMsgLookUp = 'rLkR';
const uint32 kMsgLookupDone = 'rLkD';


// A release ID is a UUID: 36 characters, dashes after the 8th, 13th, 18th
// and 23rd. Anything else is taken to be a disc ID.
bool
LooksLikeReleaseId(const BString& id)
{
	if (id.Length() != 36)
		return false;
	return id.ByteAt(8) == '-' && id.ByteAt(13) == '-' && id.ByteAt(18) == '-'
		&& id.ByteAt(23) == '-';
}


BString
DurationText(int32 seconds)
{
	BString text;
	if (seconds < 0)
		return text;
	char buffer[32];
	snprintf(buffer, sizeof(buffer), "%d:%02d", (int)(seconds / 60),
		(int)(seconds % 60));
	text = buffer;
	return text;
}


// The lookup's answer as text for the window.
BString
FormatResult(const BString& id, const tagkit::ReleaseLookupResult& result)
{
	BString text;
	switch (result.status) {
		case B_OK:
			break;
		case B_ENTRY_NOT_FOUND:
			text << "MusicBrainz has no release for \"" << id << "\".\n";
			return text;
		case B_TIMED_OUT:
			text << "The lookup timed out.\n";
			return text;
		case B_BUSY:
			text << "MusicBrainz is busy -- wait a moment and try again.\n";
			return text;
		case B_BAD_VALUE:
			text << "That isn't a disc ID or release ID.\n";
			return text;
		default:
			text << "The lookup failed (check the network connection).\n";
			return text;
	}

	text << (int32)result.releases.size()
		<< (result.releases.size() == 1 ? " release" : " releases")
		<< " found.\n";

	for (size_t r = 0; r < result.releases.size(); r++) {
		const tagkit::ReleaseInfo& release = result.releases[r];
		text << "\n";
		text << "Album:         " << release.title << "\n";
		text << "Artist:        " << release.artist << "\n";
		text << "Date:          " << release.date << "\n";
		text << "Release ID:    " << release.id << "\n";
		text << "Release group: " << release.releaseGroupId << "\n";
		text << "Artist ID:     " << release.artistId << "\n";
		if (release.discId.Length() > 0)
			text << "Disc ID:       " << release.discId << "\n";

		text << "Discs:         " << release.mediumCount;
		if (release.discNumber > 0)
			text << " (tracks below are from disc " << release.discNumber
				<< ")";
		else
			text << " (tracks below are from all discs)";
		text << "\n\n";

		for (size_t t = 0; t < release.tracks.size(); t++) {
			const tagkit::TrackInfo& track = release.tracks[t];
			if (release.discNumber == 0)
				text << "d" << track.disc << " ";
			if (track.number < 10)
				text << " ";
			text << track.number << ". " << track.title << " -- "
				<< track.artist;
			BString duration = DurationText(track.durationSeconds);
			if (duration.Length() > 0)
				text << "  [" << duration << "]";
			text << "\n      recording " << track.recordingId << "\n";
		}
	}

	return text;
}


struct LookupParams {
	BMessenger	target;
	BString		id;
	BString		artist;
	BString		album;
	int32		trackCount;
};


// The answer of tagkit::MusicBrainzSearch::LookupDisc() as text: how the
// release was found, whether its tracks fit, then the usual details.
BString
FormatDiscResult(const BString& id, const tagkit::DiscLookupResult& found)
{
	tagkit::ReleaseLookupResult asLookup;
	asLookup.status = found.status;
	if (found.status == B_OK)
		asLookup.releases.push_back(found.release);

	BString text;
	if (found.status == B_OK) {
		text << (found.foundByDiscId ? "Found by disc ID."
			: "Not found by disc ID; found by artist and album.") << "\n";
		text << (found.hasTracks
			? "The track list is that of the disc.\n"
			: "No track list: none of the releases checked has a disc with "
				"that many tracks.\n");
		text << "Cover art candidates: " << (int32)found.candidates.size()
			<< "\n";
		for (size_t i = 0; i < found.candidates.size(); i++) {
			text << "  " << found.candidates[i].title << " ("
				<< found.candidates[i].year << ") "
				<< found.candidates[i].id << "\n";
		}
		text << "\n";
	}

	text << FormatResult(id, asLookup);
	return text;
}


status_t
LookupThreadEntry(void* data)
{
	LookupParams* params = static_cast<LookupParams*>(data);

	BMessage done(kMsgLookupDone);
	if (LooksLikeReleaseId(params->id)) {
		done.AddString("text", FormatResult(params->id,
			tagkit::MusicBrainzSearch::LookupRelease(params->id)));
	} else if (params->artist.Length() > 0 || params->trackCount > 0) {
		// Also tries the artist and album when the disc ID isn't known.
		done.AddString("text", FormatDiscResult(params->id,
			tagkit::MusicBrainzSearch::LookupDisc(params->id,
				params->trackCount, params->artist, params->album)));
	} else {
		done.AddString("text", FormatResult(params->id,
			tagkit::MusicBrainzSearch::LookupDiscId(params->id)));
	}
	params->target.SendMessage(&done);

	delete params;
	return B_OK;
}

}	// namespace


ReleaseLookupWindow::ReleaseLookupWindow()
	:
	BWindow(BRect(140, 140, 700, 520), "Look Up Release ID", B_TITLED_WINDOW,
		B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS
			| B_ASYNCHRONOUS_CONTROLS),
	fLookingUp(false)
{
	fIdControl = new BTextControl("id", "ID:", "", new BMessage(kMsgLookUp));
	fIdControl->SetToolTip("A MusicBrainz disc ID, or a release ID (the long "
		"one with dashes).");

	fArtistControl = new BTextControl("artist", "Artist:", "",
		new BMessage(kMsgLookUp));
	fAlbumControl = new BTextControl("album", "Album:", "",
		new BMessage(kMsgLookUp));
	fTracksControl = new BTextControl("tracks", "Tracks:", "",
		new BMessage(kMsgLookUp));
	fArtistControl->SetToolTip("Optional. With a disc ID that MusicBrainz "
		"doesn't know, the album is looked for by artist and album instead.");
	fTracksControl->SetToolTip("Optional. How many tracks the CD has. A "
		"release found by artist and album only supplies its track list when "
		"it has a disc with this many tracks.");

	fLookUpButton = new BButton("lookup", "Look Up", new BMessage(kMsgLookUp));
	fLookUpButton->MakeDefault(true);

	fResultView = new BTextView("result");
	fResultView->MakeEditable(false);
	fResultView->SetFont(be_fixed_font);
	fResultView->SetWordWrap(false);
	_ShowText("Paste a disc ID or a release ID above and press Look Up.\n\n"
		"To try the fallback for a disc MusicBrainz doesn't know, also fill in\n"
		"the artist and album (and how many tracks the CD has), or leave the\n"
		"ID empty.");

	BScrollView* scrollView = new BScrollView("scroll", fResultView, 0, true,
		true);

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_WINDOW_INSETS)
		.AddGroup(B_HORIZONTAL)
			.Add(fIdControl)
			.Add(fLookUpButton)
		.End()
		.AddGroup(B_HORIZONTAL)
			.Add(fArtistControl)
			.Add(fAlbumControl)
			.Add(fTracksControl)
		.End()
		.Add(scrollView)
		.End();

	fIdControl->SetTarget(this);
	fArtistControl->SetTarget(this);
	fAlbumControl->SetTarget(this);
	fTracksControl->SetTarget(this);
	fLookUpButton->SetTarget(this);
	fIdControl->MakeFocus(true);
}


void
ReleaseLookupWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgLookUp:
			_StartLookup();
			break;

		case kMsgLookupDone:
		{
			BString text;
			message->FindString("text", &text);
			_ShowText(text.String());
			fLookingUp = false;
			fLookUpButton->SetEnabled(true);
			break;
		}

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void
ReleaseLookupWindow::_StartLookup()
{
	if (fLookingUp)
		return;

	BString id(fIdControl->Text());
	id.Trim();
	BString artist(fArtistControl->Text());
	artist.Trim();
	BString album(fAlbumControl->Text());
	album.Trim();
	if (id.Length() == 0 && artist.Length() == 0)
		return;

	LookupParams* params = new LookupParams;
	params->target = BMessenger(this);
	params->id = id;
	params->artist = artist;
	params->album = album;
	params->trackCount = atoi(fTracksControl->Text());

	thread_id thread = spawn_thread(LookupThreadEntry, "release lookup",
		B_NORMAL_PRIORITY, params);
	if (thread < 0) {
		delete params;
		_ShowText("Couldn't start the lookup.");
		return;
	}

	fLookingUp = true;
	fLookUpButton->SetEnabled(false);
	_ShowText("Looking up" B_UTF8_ELLIPSIS);
	resume_thread(thread);
}


void
ReleaseLookupWindow::_ShowText(const char* text)
{
	fResultView->SetText(text);
	fResultView->ScrollToOffset(0);
}
