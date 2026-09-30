/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include "TagViewWindow.h"

#include <Alert.h>
#include <Alignment.h>
#include <AppDefs.h>
#include <Application.h>
#include <Entry.h>
#include <FilePanel.h>
#include <LayoutBuilder.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Message.h>
#include <Messenger.h>
#include <Node.h>
#include <OS.h>
#include <Path.h>
#include <String.h>
#include <StringView.h>
#include <stdio.h>

#include "Messages.h"
#include "SearchResultsWindow.h"
#include "SearchWindow.h"
#include "tagkit/MusicBrainzSearch.h"
#include "tagkit/RecordingMatch.h"
#include "tagkit/TagRecord.h"
#include "tagkit/TagView.h"
#include "widgetkit/Barberpole.h"

using tagkit::RecordingMatch;
using tagkit::TagRecord;
using tagkit::TagRow;


namespace {

// Runs tagkit::MusicBrainzSearch::SearchRecording() on its own thread
// (it's a network call, so it can't run on the UI thread) and reports
// the matches back as a kMsgSearchCompleted message: the original query
// ("artist"/"song") plus each match's fields as parallel indexed arrays
// ("matchId"/"matchTitle"/"matchArtist"/"matchAlbum"/
// "matchDurationSeconds"/"matchScore") -- same "parallel arrays for a
// list of results" idiom Hare's own MusicBrainz code uses for messages
// like this.
struct SearchThreadParams {
	BMessenger	target;
	BString		artist;
	BString		song;
	int32		durationSeconds;	// -1 == no track time given
};


status_t
SearchThreadEntry(void* data)
{
	SearchThreadParams* params = static_cast<SearchThreadParams*>(data);

	std::vector<RecordingMatch> matches
		= tagkit::MusicBrainzSearch::SearchRecording(params->artist,
			params->song, 10, params->durationSeconds);

	BMessage result(kMsgSearchCompleted);
	result.AddString("artist", params->artist);
	result.AddString("song", params->song);

	for (size_t i = 0; i < matches.size(); i++) {
		const RecordingMatch& match = matches[i];
		result.AddString("matchId", match.id);
		result.AddString("matchTitle", match.title);
		result.AddString("matchArtist", match.artist);
		result.AddString("matchAlbum", match.album);
		result.AddInt32("matchDurationSeconds", match.durationSeconds);
		result.AddInt32("matchScore", match.score);
	}

	params->target.SendMessage(&result);

	delete params;
	return B_OK;
}

} // namespace


namespace {

// Restricts the Open... file panel to directories (for navigation) and
// files with an extension tagkit recognizes (.mp3, .ogg, .flac).
class AudioRefFilter : public BRefFilter {
public:
	virtual bool Filter(const entry_ref* ref, BNode* node,
		struct stat_beos* stat, const char* fileType)
	{
		// stat_beos is an opaque BeOS-compat type here (only forward
		// declared), so we can't read st_mode out of it directly --
		// BNode::IsDirectory() (from BStatable) does the equivalent
		// check safely.
		if (node != NULL && node->IsDirectory())
			return true;

		BString name(ref->name);
		return tagkit::format_from_extension(name)
			!= tagkit::AUDIO_FORMAT_UNKNOWN;
	}
};

} // namespace


static const float kWindowWidth = 720;
static const float kWindowHeight = 420;


TagViewWindow::TagViewWindow()
	:
	BWindow(BRect(80, 80, 80 + kWindowWidth, 80 + kWindowHeight),
		"TagView", B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS),
	fTagView(NULL),
	fBusyIndicator(NULL),
	fStatusView(NULL),
	fOpenPanel(NULL),
	fSearchWindow(NULL),
	fResultsWindow(NULL),
	fSearchTargetRow(NULL)
{
	BMenuBar* menuBar = _BuildMenuBar();

	fTagView = new tagkit::TagView("tagListView");

	fBusyIndicator = new Barberpole("busyIndicator", B_WILL_DRAW);
	fBusyIndicator->SetExplicitMinSize(BSize(90, 20));
	fBusyIndicator->SetExplicitMaxSize(BSize(90, 20));

	fStatusView = new BStringView("statusView", "Ready.");
	fStatusView->SetAlignment(B_ALIGN_LEFT);
	fStatusView->SetExplicitAlignment(
		BAlignment(B_ALIGN_LEFT, B_ALIGN_VERTICAL_CENTER));

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.Add(menuBar)
		.Add(fTagView)
		.AddGroup(B_HORIZONTAL, B_USE_SMALL_SPACING)
			.SetInsets(B_USE_SMALL_SPACING, B_USE_SMALL_SPACING,
				B_USE_SMALL_SPACING, B_USE_SMALL_SPACING)
			.Add(fBusyIndicator, 0.0f)
			.Add(fStatusView, 1.0f)
		.End()
		.End();

	fOpenPanel = new BFilePanel(B_OPEN_PANEL, new BMessenger(this), NULL,
		B_FILE_NODE, true /* allowMultipleSelection */, NULL,
		new AudioRefFilter(), false /* modal */, true /* hideWhenDone */);
}


TagViewWindow::~TagViewWindow()
{
	delete fOpenPanel;
}


BMenuBar*
TagViewWindow::_BuildMenuBar()
{
	BMenuBar* menuBar = new BMenuBar("menuBar");

	BMenu* fileMenu = new BMenu("File");
	fileMenu->AddItem(new BMenuItem("Open" B_UTF8_ELLIPSIS,
		new BMessage(kMsgFileOpen), 'O'));
	fileMenu->AddSeparatorItem();
	fileMenu->AddItem(new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED),
		'Q'));
	menuBar->AddItem(fileMenu);

	BMenu* editMenu = new BMenu("Edit");
	editMenu->AddItem(new BMenuItem("Search" B_UTF8_ELLIPSIS,
		new BMessage(kMsgEditSearch), 'F'));
	menuBar->AddItem(editMenu);

	BMenu* helpMenu = new BMenu("Help");
	helpMenu->AddItem(new BMenuItem("About TagView" B_UTF8_ELLIPSIS,
		new BMessage(kMsgHelpAbout)));
	menuBar->AddItem(helpMenu);

	return menuBar;
}


void
TagViewWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgFileOpen:
			fOpenPanel->Show();
			break;

		case B_REFS_RECEIVED:
		case B_SIMPLE_DATA:
			// B_SIMPLE_DATA covers refs dragged straight from Tracker
			// onto the window/view rather than chosen via the file panel.
			_AddRefs(message);
			break;

		case kMsgEditSearch:
			_HandleEditSearch();
			break;

		case kMsgSearchRequested:
			_HandleSearchRequested(message);
			break;

		case kMsgSearchCompleted:
			_HandleSearchCompleted(message);
			break;

		case kMsgSearchWindowClosed:
			fSearchWindow = NULL;
			break;

		case kMsgApplyMatch:
			_HandleApplyMatch(message);
			break;

		case kMsgResultsWindowClosed:
			fResultsWindow = NULL;
			break;

		case kMsgHelpAbout:
			_ShowAbout();
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


bool
TagViewWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


void
TagViewWindow::_AddRefs(BMessage* message)
{
	entry_ref ref;
	int32 index = 0;

	while (message->FindRef("refs", index++, &ref) == B_OK) {
		BEntry entry(&ref, true /* traverse links */);
		if (!entry.Exists() || entry.IsDirectory())
			continue;

		BPath path;
		if (entry.GetPath(&path) != B_OK)
			continue;

		TagRecord record;
		record.path = path.Path();
		record.fileName = ref.name;
		record.format = tagkit::format_from_extension(record.fileName);

		if (record.format == tagkit::AUDIO_FORMAT_UNKNOWN)
			continue;

		// TODO: read real tag data via TagLib here. For now rows are
		// added with the tag fields empty, which IsUntagged() will
		// pick up so the Search... window has something to work with.
		record.tagsLoaded = false;

		fTagView->AddTag(record);
	}
}


void
TagViewWindow::_HandleEditSearch()
{
	// Remember (and pre-fill from) whatever's selected right now, so
	// _HandleApplyMatch() later knows which row a chosen match belongs
	// to. NULL (nothing selected) is fine -- search still works, the
	// result just can't be applied back to a row.
	fSearchTargetRow = fTagView->SelectedRow();

	if (fSearchWindow == NULL)
		fSearchWindow = new SearchWindow(BMessenger(this));

	if (fSearchTargetRow != NULL) {
		const TagRecord& record = fSearchTargetRow->Record();
		BString artist, song;
		if (record.IsUntagged()
				&& tagkit::guess_artist_song_from_file_name(record.fileName,
					artist, song)) {
			fSearchWindow->SetQuery(artist.String(), song.String(),
				record.durationSeconds);
		} else if (!record.IsUntagged()) {
			fSearchWindow->SetQuery(record.artist.String(),
				record.title.String(), record.durationSeconds);
		}
	}

	if (fSearchWindow->IsHidden())
		fSearchWindow->Show();
	else
		fSearchWindow->MoveToFrontAndFocus();
}


void
TagViewWindow::_HandleSearchRequested(BMessage* message)
{
	BString artist, song;
	message->FindString("artist", &artist);
	message->FindString("song", &song);

	// Optional: only present when a track time was entered.
	int32 durationSeconds = -1;
	if (message->FindInt32("durationSeconds", &durationSeconds) != B_OK)
		durationSeconds = -1;

	BString status("Searching MusicBrainz for \"");
	status << artist << "\" - \"" << song << "\"";
	if (durationSeconds >= 0) {
		char time[16];
		snprintf(time, sizeof(time), " (%d:%02d)", (int)(durationSeconds / 60),
			(int)(durationSeconds % 60));
		status << time;
	}
	status << B_UTF8_ELLIPSIS;
	_SetStatus(true, status.String());

	SearchThreadParams* params = new SearchThreadParams;
	params->target = BMessenger(this);
	params->artist = artist;
	params->song = song;
	params->durationSeconds = durationSeconds;

	thread_id thread = spawn_thread(SearchThreadEntry, "musicbrainz search",
		B_NORMAL_PRIORITY, params);
	if (thread < 0) {
		delete params;
		_SetStatus(false, "Couldn't start the search.");
		return;
	}
	resume_thread(thread);
}


void
TagViewWindow::_HandleSearchCompleted(BMessage* message)
{
	BString artist, song;
	message->FindString("artist", &artist);
	message->FindString("song", &song);

	std::vector<RecordingMatch> matches;
	BString matchId, matchTitle, matchArtist, matchAlbum;
	for (int32 i = 0; message->FindString("matchId", i, &matchId) == B_OK;
			i++) {
		RecordingMatch match;
		match.id = matchId;
		message->FindString("matchTitle", i, &matchTitle);
		message->FindString("matchArtist", i, &matchArtist);
		message->FindString("matchAlbum", i, &matchAlbum);
		match.title = matchTitle;
		match.artist = matchArtist;
		match.album = matchAlbum;
		message->FindInt32("matchDurationSeconds", i,
			&match.durationSeconds);
		message->FindInt32("matchScore", i, &match.score);
		matches.push_back(match);
	}

	if (matches.empty()) {
		BString status("No MusicBrainz matches found for \"");
		status << artist << "\" - \"" << song << "\".";
		_SetStatus(false, status.String());
		return;
	}

	BString status;
	status << (int32)matches.size()
		<< (matches.size() == 1 ? " match" : " matches")
		<< " found for \"" << artist << "\" - \"" << song << "\".";
	_SetStatus(false, status.String());

	if (fResultsWindow == NULL) {
		fResultsWindow = new SearchResultsWindow(BMessenger(this), artist,
			song);
	}
	fResultsWindow->SetMatches(matches);

	if (fResultsWindow->IsHidden())
		fResultsWindow->Show();
	else
		fResultsWindow->MoveToFrontAndFocus();
}


void
TagViewWindow::_HandleApplyMatch(BMessage* message)
{
	if (fSearchTargetRow == NULL) {
		_SetStatus(false, "No file was selected to apply that match to.");
		return;
	}

	TagRecord record = fSearchTargetRow->Record();

	BString title, artist, album;
	message->FindString("title", &title);
	message->FindString("artist", &artist);
	message->FindString("album", &album);

	record.title = title;
	record.artist = artist;
	record.album = album;
	message->FindInt32("durationSeconds", &record.durationSeconds);

	// This only updates what TagView displays; writing the match back
	// into the file's actual tags (via TagLib) is a separate step still
	// to come.
	fTagView->UpdateRow(fSearchTargetRow, record);

	BString status("Applied MusicBrainz match to \"");
	status << record.fileName << "\".";
	_SetStatus(false, status.String());
}


void
TagViewWindow::_SetStatus(bool busy, const char* text)
{
	if (busy)
		fBusyIndicator->Start();
	else
		fBusyIndicator->Stop();

	if (text != NULL)
		fStatusView->SetText(text);
}


void
TagViewWindow::_ShowAbout()
{
	BAlert* alert = new BAlert("About TagView",
		"TagView\n\n"
		"A music tag viewer for Haiku, built around a reusable "
		"\"tagkit\" library (tag data model, a BColumnListView-based tag "
		"list widget, and a MusicBrainz recording search) intended to be "
		"shared with other apps such as Hare and ArmyKnife.\n\n"
		"Tag reading and writing (TagLib) and cover art (libcoverart) "
		"are on the way.",
		"OK");
	alert->Go();
}
