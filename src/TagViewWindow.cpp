/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
#include "TagViewWindow.h"

#include <memory>
#include <vector>

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

#include "CoverArtPickerWindow.h"
#include "Messages.h"
#include "SearchResultsWindow.h"
#include "SearchWindow.h"
#include "tagkit/CoverArtFetch.h"
#include "tagkit/CoverArtImage.h"
#include "tagkit/CoverArtView.h"
#include "tagkit/MusicBrainzSearch.h"
#include "tagkit/RecordingMatch.h"
#include "tagkit/TagReader.h"
#include "tagkit/TagRecord.h"
#include "tagkit/TagView.h"
#include "tagkit/TagWriter.h"
#include "widgetkit/Barberpole.h"

using tagkit::CoverArtImage;
using tagkit::RecordingMatch;
using tagkit::ReleaseRef;
using tagkit::TagRecord;
using tagkit::TagRow;


namespace {

// Runs tagkit::MusicBrainzSearch::SearchRecording() on its own thread
// (it's a network call, so it can't run on the UI thread) and reports
// the matches back as a kMsgSearchCompleted message: the original query
// ("artist"/"song") plus each match's fields as parallel indexed arrays
// ("matchId"/"matchTitle"/"matchArtist"/"matchAlbum"/
// "matchDurationSeconds"/"matchTrack"/"matchYear"/"matchScore") -- same "parallel arrays for a
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
	if (params->durationSeconds >= 0)
		result.AddInt32("durationSeconds", params->durationSeconds);

	for (size_t i = 0; i < matches.size(); i++) {
		const RecordingMatch& match = matches[i];
		result.AddString("matchId", match.id);
		result.AddString("matchTitle", match.title);
		result.AddString("matchArtist", match.artist);
		result.AddString("matchAlbum", match.album);
		result.AddInt32("matchDurationSeconds", match.durationSeconds);
		result.AddInt32("matchTrack", match.track);
		result.AddInt32("matchYear", match.year);

		// Each match's releases ride along as a nested message holding
		// parallel "id"/"title"/"year" arrays.
		BMessage releases;
		for (size_t r = 0; r < match.releases.size(); r++) {
			releases.AddString("id", match.releases[r].id);
			releases.AddString("title", match.releases[r].title);
			releases.AddInt32("year", match.releases[r].year);
		}
		result.AddMessage("matchReleases", &releases);
		result.AddInt32("matchScore", match.score);
	}

	params->target.SendMessage(&result);

	delete params;
	return B_OK;
}


// Same idea for the cover art lookup: one Cover Art Archive request per
// release is slow, so it runs on its own thread and reports back with a
// kMsgCoverArtFetched message (the echoed "requestId", plus each image's
// "releaseId"/"releaseTitle"/"releaseYear"/"mimeType"/"imageData" as
// parallel indexed arrays, "imageData" being the raw compressed bytes).
struct CoverArtThreadParams {
	BMessenger				target;
	std::vector<ReleaseRef>	releases;
	int32					requestId;
};


status_t
CoverArtThreadEntry(void* data)
{
	CoverArtThreadParams* params = static_cast<CoverArtThreadParams*>(data);

	std::vector<CoverArtImage> images
		= tagkit::fetch_cover_art(params->releases);

	BMessage result(kMsgCoverArtFetched);
	result.AddInt32("requestId", params->requestId);
	result.AddInt32("releasesChecked", (int32)params->releases.size());

	for (size_t i = 0; i < images.size(); i++) {
		const CoverArtImage& image = images[i];
		result.AddString("releaseId", image.releaseId);
		result.AddString("releaseTitle", image.releaseTitle);
		result.AddInt32("releaseYear", image.year);
		result.AddString("mimeType", image.mimeType);
		result.AddData("imageData", B_RAW_TYPE, &image.data[0],
			image.data.size());
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
static const float kWindowHeight = 560;


TagViewWindow::TagViewWindow()
	:
	BWindow(BRect(80, 80, 80 + kWindowWidth, 80 + kWindowHeight),
		"TagView", B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS),
	fTagView(NULL),
	fCoverArtView(NULL),
	fCoverArtShownRow(NULL),
	fBusyIndicator(NULL),
	fStatusView(NULL),
	fOpenPanel(NULL),
	fSearchWindow(NULL),
	fResultsWindow(NULL),
	fSearchTargetRow(NULL),
	fCoverArtWindow(NULL),
	fCoverArtTargetRow(NULL),
	fCoverArtRequestId(0),
	fCoverArtReleasesChecked(0)
{
	BMenuBar* menuBar = _BuildMenuBar();

	fTagView = new tagkit::TagView("tagListView");
	fTagView->SetSelectionChangedMessage(new BMessage(kMsgSelectionChanged));

	fCoverArtView = new tagkit::CoverArtView("coverArtView");

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
			.Add(fCoverArtView, 0.0f)
			.AddGlue()
		.End()
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
	fileMenu->AddItem(new BMenuItem("Save", new BMessage(kMsgFileSave), 'S'));
	fileMenu->AddItem(new BMenuItem("Save All", new BMessage(kMsgFileSaveAll),
		'S', B_SHIFT_KEY));
	fileMenu->AddSeparatorItem();
	fileMenu->AddItem(new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED),
		'Q'));
	menuBar->AddItem(fileMenu);

	BMenu* editMenu = new BMenu("Edit");
	editMenu->AddItem(new BMenuItem("Search" B_UTF8_ELLIPSIS,
		new BMessage(kMsgEditSearch), 'F'));
	editMenu->AddItem(new BMenuItem("Choose Cover Art" B_UTF8_ELLIPSIS,
		new BMessage(kMsgEditChooseCoverArt)));
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

		case kMsgFileSave:
			_HandleSave();
			break;

		case kMsgFileSaveAll:
			_HandleSaveAll();
			break;

		case kMsgEditSearch:
			_HandleEditSearch();
			break;

		case kMsgEditChooseCoverArt:
			_HandleChooseCoverArt();
			break;

		case kMsgSelectionChanged:
			_HandleSelectionChanged();
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

		case kMsgCoverArtFetched:
			_HandleCoverArtFetched(message);
			break;

		case kMsgCoverArtChosen:
			_HandleCoverArtChosen(message);
			break;

		case kMsgCoverArtWindowClosed:
			fCoverArtWindow = NULL;
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
	// Don't silently throw away tag changes that were never written.
	int32 modified = _CountModifiedRows();
	if (modified > 0) {
		BString text;
		if (modified == 1)
			text << "1 file has";
		else
			text << modified << " files have";
		text << " tag changes that haven't been saved to disk.";

		BAlert* alert = new BAlert("unsaved", text.String(), "Cancel",
			"Discard Changes", "Save All", B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		alert->SetShortcut(0, B_ESCAPE);
		int32 choice = alert->Go();

		if (choice == 0)
			return false;

		if (choice == 2) {
			_HandleSaveAll();
			// If anything couldn't be saved, stay open rather than
			// losing it on the way out.
			if (_CountModifiedRows() > 0)
				return false;
		}
	}

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

		// Read the file's tags and audio properties with TagLib. If that
		// fails the row is still added (file name and format only), and
		// IsUntagged() will pick it up so Search... can still offer a
		// guess based on the file name.
		tagkit::read_tags(record);

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

	// Optional: only present when the search included a track time.
	int32 durationSeconds = -1;
	if (message->FindInt32("durationSeconds", &durationSeconds) != B_OK)
		durationSeconds = -1;

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
		message->FindInt32("matchTrack", i, &match.track);
		message->FindInt32("matchYear", i, &match.year);

		BMessage releases;
		if (message->FindMessage("matchReleases", i, &releases) == B_OK) {
			BString releaseId, releaseTitle;
			for (int32 r = 0; releases.FindString("id", r, &releaseId) == B_OK;
					r++) {
				ReleaseRef ref;
				ref.id = releaseId;
				if (releases.FindString("title", r, &releaseTitle) == B_OK)
					ref.title = releaseTitle;
				releases.FindInt32("year", r, &ref.year);
				match.releases.push_back(ref);
			}
		}
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
			song, durationSeconds);
	} else {
		// Reused window: refresh the title for this search's terms.
		fResultsWindow->SetQuery(artist, song, durationSeconds);
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
	record.modified = true;

	// Track and year only come across when the match actually has them
	// (0 == MusicBrainz didn't say), so an unknown never blanks out a
	// value the file already had.
	int32 matchTrack = 0, matchYear = 0;
	message->FindInt32("track", &matchTrack);
	message->FindInt32("year", &matchYear);
	if (matchTrack > 0)
		record.track = matchTrack;
	if (matchYear > 0)
		record.year = matchYear;

	// The file's own length (read from the file) is more trustworthy than
	// MusicBrainz's, so only fall back to the match's when we don't have
	// one.
	int32 matchDuration;
	if (record.durationSeconds < 0
			&& message->FindInt32("durationSeconds", &matchDuration) == B_OK)
		record.durationSeconds = matchDuration;

	// This only updates the row (marked as modified); File > Save is what
	// writes it to the file's actual tags.
	fTagView->UpdateRow(fSearchTargetRow, record);

	BString status("Applied MusicBrainz match to \"");
	status << record.fileName << "\" (not saved yet -- File > Save writes "
		"it to the file).";

	// Look for cover art on the releases this recording appears on.
	std::vector<ReleaseRef> releases;
	BString releaseId, releaseTitle;
	for (int32 i = 0; message->FindString("releaseId", i, &releaseId) == B_OK;
			i++) {
		ReleaseRef ref;
		ref.id = releaseId;
		if (message->FindString("releaseTitle", i, &releaseTitle) == B_OK)
			ref.title = releaseTitle;
		message->FindInt32("releaseYear", i, &ref.year);
		releases.push_back(ref);
	}

	if (releases.empty()) {
		_SetStatus(false, status.String());
		return;
	}

	status << " Looking for cover art" B_UTF8_ELLIPSIS;
	_StartCoverArtFetch(fSearchTargetRow, releases, status.String());
}


void
TagViewWindow::_StartCoverArtFetch(tagkit::TagRow* row,
	const std::vector<ReleaseRef>& releases, const char* statusText)
{
	fCoverArtTargetRow = row;
	fCoverArtRequestId++;

	CoverArtThreadParams* params = new CoverArtThreadParams;
	params->target = BMessenger(this);
	params->releases = releases;
	params->requestId = fCoverArtRequestId;

	thread_id thread = spawn_thread(CoverArtThreadEntry, "cover art lookup",
		B_NORMAL_PRIORITY, params);
	if (thread < 0) {
		delete params;
		_SetStatus(false, "Couldn't start the cover art lookup.");
		return;
	}

	_SetStatus(true, statusText);
	resume_thread(thread);
}


void
TagViewWindow::_HandleCoverArtFetched(BMessage* message)
{
	// A newer lookup has started since this one; its answer is the one
	// that counts.
	int32 requestId = 0;
	message->FindInt32("requestId", &requestId);
	if (requestId != fCoverArtRequestId)
		return;

	std::vector<CoverArtImage> images;
	BString releaseId, releaseTitle, mimeType;
	for (int32 i = 0; message->FindString("releaseId", i, &releaseId) == B_OK;
			i++) {
		CoverArtImage image;
		image.releaseId = releaseId;
		if (message->FindString("releaseTitle", i, &releaseTitle) == B_OK)
			image.releaseTitle = releaseTitle;
		message->FindInt32("releaseYear", i, &image.year);
		if (message->FindString("mimeType", i, &mimeType) == B_OK)
			image.mimeType = mimeType;

		const void* bytes = NULL;
		ssize_t size = 0;
		if (message->FindData("imageData", B_RAW_TYPE, i, &bytes, &size)
				!= B_OK || size <= 0) {
			continue;
		}
		const unsigned char* begin = static_cast<const unsigned char*>(bytes);
		image.data.assign(begin, begin + size);
		images.push_back(image);
	}

	fCoverArtReleasesChecked = 0;
	message->FindInt32("releasesChecked", &fCoverArtReleasesChecked);

	if (images.empty()) {
		BString status("No cover art found on any of the ");
		status << fCoverArtReleasesChecked << " releases checked.";
		_SetStatus(false, status.String());
		return;
	}

	if (fCoverArtTargetRow == NULL) {
		_SetStatus(false, "Cover art found, but there's no file to use it "
			"for.");
		return;
	}

	// One candidate (the common case): just use it, as Hare does. Several:
	// let the user choose.
	fCoverArtCandidates = images;

	if (images.size() == 1) {
		_SetCoverArt(fCoverArtTargetRow, images[0]);

		BString status("Found cover art on 1 of ");
		status << fCoverArtReleasesChecked << " releases -- using it for \""
			<< fCoverArtTargetRow->Record().fileName << "\" (not saved yet "
			"-- File > Save writes it to the file).";
		_SetStatus(false, status.String());
		return;
	}

	BString status("Found cover art on ");
	status << (int32)images.size() << " of " << fCoverArtReleasesChecked
		<< " releases -- choose one.";
	_SetStatus(false, status.String());

	_ShowCoverArtPicker();
}


void
TagViewWindow::_ShowCoverArtPicker()
{
	if (fCoverArtTargetRow == NULL || fCoverArtCandidates.empty())
		return;

	// Replace a picker left open from an earlier lookup. Quit() (unlike
	// closing it) doesn't send us the "window closed" message, so there's
	// no stale one to clobber the new pointer below.
	if (fCoverArtWindow != NULL && fCoverArtWindow->Lock())
		fCoverArtWindow->Quit();

	fCoverArtWindow = new CoverArtPickerWindow(BMessenger(this),
		fCoverArtTargetRow->Record().fileName, fCoverArtCandidates);
	fCoverArtWindow->Show();
}


void
TagViewWindow::_HandleChooseCoverArt()
{
	if (fCoverArtTargetRow == NULL || fCoverArtCandidates.size() < 2) {
		_SetStatus(false, "No cover art alternatives to choose from yet -- "
			"apply a MusicBrainz match first; if its releases have several "
			"covers you'll get to pick.");
		return;
	}

	_ShowCoverArtPicker();
}


void
TagViewWindow::_HandleSelectionChanged()
{
	// Show the art of whichever row was selected last; clearing the
	// selection leaves the last one showing.
	tagkit::TagRow* row = fTagView->SelectedRow();
	if (row == NULL || row == fCoverArtShownRow)
		return;

	fCoverArtShownRow = row;
	_RefreshCoverArt();
}


void
TagViewWindow::_RefreshCoverArt()
{
	if (fCoverArtShownRow == NULL) {
		fCoverArtView->Clear();
		return;
	}

	// Art waiting to be saved wins over what's in the file.
	const TagRecord& record = fCoverArtShownRow->Record();
	CoverArtImage image;
	bool haveImage = false;
	if (record.newCoverArt != NULL) {
		image = *record.newCoverArt;
		haveImage = true;
	} else if (record.hasCoverArt) {
		haveImage = tagkit::read_cover_art(record.path, image);
	}

	// The view takes ownership of the bitmap (NULL shows its placeholder).
	fCoverArtView->SetBitmap(haveImage ? tagkit::decode_cover_art(image)
		: NULL);
}


void
TagViewWindow::_HandleCoverArtChosen(BMessage* message)
{
	int32 index = -1;
	if (message->FindInt32("index", &index) != B_OK || index < 0
			|| index >= (int32)fCoverArtCandidates.size()
			|| fCoverArtTargetRow == NULL) {
		return;
	}

	_SetCoverArt(fCoverArtTargetRow, fCoverArtCandidates[index]);
}


void
TagViewWindow::_SetCoverArt(tagkit::TagRow* row, const CoverArtImage& image)
{
	TagRecord record = row->Record();
	record.newCoverArt = std::make_shared<const CoverArtImage>(image);
	record.modified = true;
	fTagView->UpdateRow(row, record);
	if (row == fCoverArtShownRow)
		_RefreshCoverArt();

	BString status("Chose cover art");
	if (!image.releaseTitle.IsEmpty())
		status << " from \"" << image.releaseTitle << "\"";
	status << " for \"" << record.fileName << "\" (not saved yet -- "
		"File > Save writes it to the file).";
	_SetStatus(false, status.String());
}


int32
TagViewWindow::_CountModifiedRows() const
{
	int32 count = 0;
	for (int32 i = 0; i < fTagView->CountRows(); i++) {
		tagkit::TagRow* row = static_cast<tagkit::TagRow*>(fTagView->RowAt(i));
		if (row->Record().modified)
			count++;
	}
	return count;
}


bool
TagViewWindow::_SaveRow(tagkit::TagRow* row, BString& error)
{
	TagRecord record = row->Record();

	if (!tagkit::write_tags(record, &error))
		return false;

	// Re-read what's now on disk so the row shows what was actually
	// saved (and the length etc. stay in step with the file). If the
	// re-read somehow fails the in-memory values are what we just wrote
	// anyway.
	record.newCoverArt.reset();
	record.modified = false;
	tagkit::read_tags(record);
	record.modified = false;

	fTagView->UpdateRow(row, record);
	if (row == fCoverArtShownRow)
		_RefreshCoverArt();
	return true;
}


void
TagViewWindow::_HandleSave()
{
	tagkit::TagRow* row = fTagView->SelectedRow();
	if (row == NULL) {
		_SetStatus(false, "Select a file to save.");
		return;
	}

	BString fileName = row->Record().fileName;
	if (!row->Record().modified) {
		BString status("No changes to save in \"");
		status << fileName << "\".";
		_SetStatus(false, status.String());
		return;
	}

	BString error;
	BString status;
	if (_SaveRow(row, error)) {
		status << "Saved tags to \"" << fileName << "\".";
	} else {
		status << "Couldn't save \"" << fileName << "\": " << error << ".";
		BAlert* alert = new BAlert("saveFailed", status.String(), "OK",
			NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		alert->Go(NULL);
	}
	_SetStatus(false, status.String());
}


void
TagViewWindow::_HandleSaveAll()
{
	int32 saved = 0;
	int32 failed = 0;
	BString failures;

	for (int32 i = 0; i < fTagView->CountRows(); i++) {
		tagkit::TagRow* row = static_cast<tagkit::TagRow*>(fTagView->RowAt(i));
		if (!row->Record().modified)
			continue;

		BString fileName = row->Record().fileName;
		BString error;
		if (_SaveRow(row, error)) {
			saved++;
		} else {
			failed++;
			failures << "\n" << fileName << ": " << error;
		}
	}

	BString status;
	if (saved == 0 && failed == 0) {
		status = "No changes to save.";
	} else {
		status << "Saved " << saved << (saved == 1 ? " file" : " files");
		if (failed > 0)
			status << "; " << failed << " failed";
		status << ".";
	}
	_SetStatus(false, status.String());

	if (failed > 0) {
		BString text;
		text << "Some files couldn't be saved:" << failures;
		BAlert* alert = new BAlert("saveFailed", text.String(), "OK", NULL,
			NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		alert->Go(NULL);
	}
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
