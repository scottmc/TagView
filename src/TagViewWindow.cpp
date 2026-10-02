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
#include <Button.h>
#include <AppDefs.h>
#include <Application.h>
#include <Entry.h>
#include <FilePanel.h>
#include <GroupLayout.h>
#include <GroupView.h>
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
#include "FieldEditorWindow.h"
#include "Messages.h"
#include "SearchResultsWindow.h"
#include "SearchWindow.h"
#include "tagkit/CompactView.h"
#include "tagkit/CoverArtFetch.h"
#include "tagkit/CoverArtImage.h"
#include "tagkit/ITunesArtwork.h"
#include "tagkit/CoverArtView.h"
#include "tagkit/MusicBrainzSearch.h"
#include "tagkit/RecordingMatch.h"
#include "tagkit/TagField.h"
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
	BString		album;				// empty == no album given
	int32		durationSeconds;	// -1 == no track time given
};


status_t
SearchThreadEntry(void* data)
{
	SearchThreadParams* params = static_cast<SearchThreadParams*>(data);

	std::vector<RecordingMatch> matches
		= tagkit::MusicBrainzSearch::SearchRecording(params->artist,
			params->song, params->album, 10, params->durationSeconds);

	BMessage result(kMsgSearchCompleted);
	result.AddString("artist", params->artist);
	result.AddString("song", params->song);
	if (params->album.Length() > 0)
		result.AddString("album", params->album);
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


// The lookup stops once this many covers have been found.
const int32 kMaxCoverArtCandidates = 12;

// With more covers than this, the picker opens as soon as they pile up
// (showing the first ones) and later covers are added to it as they
// arrive. With this many or fewer the picker waits for the whole lookup,
// since that's usually quick and they may turn out to be just one.
const int32 kCoverArtPickerThreshold = 3;

// Below this many covers from the Cover Art Archive, iTunes is asked too
// (and adds at most kMaxITunesCovers more).
const int32 kITunesFallbackBelow = 3;
const int32 kMaxITunesCovers = 6;

// Longest status-bar message shown, in characters; longer text is cut with
// an ellipsis (the full text is still in the tooltip).
const int32 kMaxStatusChars = 80;


// Same idea for the cover art lookup: one Cover Art Archive request per
// release is slow, so it runs on its own thread. Each cover is reported the
// moment it's fetched as a kMsgCoverArtImageFound message ("requestId" plus
// "releaseId"/"releaseTitle"/"releaseYear"/"mimeType"/"imageData", the last
// being the raw compressed bytes), so the first ones can be shown while the
// rest are still coming; a final kMsgCoverArtFetched ("requestId",
// "releasesChecked") says it's done.
//
// If the Cover Art Archive turns up fewer than kITunesFallbackBelow covers
// (many releases have none, and some downloads fail), the same thread then
// asks iTunes for the album's cover (artist/album below) and reports what
// it finds the same way.
struct CoverArtThreadParams {
	BMessenger				target;
	std::vector<ReleaseRef>	releases;
	BString					artist;
	BString					album;
	int32					requestId;
};


status_t
CoverArtThreadEntry(void* data)
{
	CoverArtThreadParams* params = static_cast<CoverArtThreadParams*>(data);

	tagkit::CoverArtFoundFunction report = [params](const CoverArtImage& image) {
		BMessage found(kMsgCoverArtImageFound);
		found.AddInt32("requestId", params->requestId);
		found.AddString("releaseId", image.releaseId);
		found.AddString("releaseTitle", image.releaseTitle);
		found.AddInt32("releaseYear", image.year);
		found.AddString("mimeType", image.mimeType);
		found.AddData("imageData", B_RAW_TYPE, &image.data[0],
			image.data.size());
		params->target.SendMessage(&found);
	};

	int32 found = tagkit::fetch_cover_art(params->releases,
		kMaxCoverArtCandidates, report);

	if (found < kITunesFallbackBelow) {
		tagkit::fetch_itunes_cover_art(params->artist, params->album,
			kMaxITunesCovers, report);
	}

	BMessage done(kMsgCoverArtFetched);
	done.AddInt32("requestId", params->requestId);
	done.AddInt32("releasesChecked", (int32)params->releases.size());
	params->target.SendMessage(&done);

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
	fPreviewGroup(NULL),
	fCompactView(NULL),
	fViewMenu(NULL),
	fDiscardButton(NULL),
	fApplyButton(NULL),
	fCompactButtonGroup(NULL),
	fCompactDiscardButton(NULL),
	fCompactApplyButton(NULL),
	fFieldEditor(NULL),
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
	fCoverArtReleasesChecked(0),
	fCoverArtPickerOpened(false)
{
	BMenuBar* menuBar = _BuildMenuBar();

	fTagView = new tagkit::TagView("tagListView");
	fTagView->SetSelectionChangedMessage(new BMessage(kMsgSelectionChanged));
	fTagView->SetEditMessage(new BMessage(kMsgFieldEditRequested));

	fCoverArtView = new tagkit::CoverArtView("coverArtView");

	// The cover preview sits in its own group view so it can be hidden
	// along with the list when the compact view is chosen.
	fPreviewGroup = new BGroupView(B_HORIZONTAL, B_USE_SMALL_SPACING);
	fDiscardButton = new BButton("discard", "Discard Changes",
		new BMessage(kMsgEditDiscard));
	fApplyButton = new BButton("apply", "Apply", new BMessage(kMsgEditApply));

	BLayoutBuilder::Group<>(fPreviewGroup)
		.SetInsets(B_USE_SMALL_SPACING, B_USE_SMALL_SPACING,
			B_USE_SMALL_SPACING, B_USE_SMALL_SPACING)
		.Add(fCoverArtView, 0.0f)
		.AddGlue()
		.Add(fDiscardButton)
		.Add(fApplyButton)
		.End();

	fCompactView = new tagkit::CompactView("compactView");
	fCompactView->SetEditMessage(new BMessage(kMsgFieldEditRequested));
	fCompactView->Hide();

	// The compact view's own Apply / Discard Changes, under its rectangle
	// at the lower right.
	fCompactDiscardButton = new BButton("compactDiscard", "Discard Changes",
		new BMessage(kMsgEditDiscard));
	fCompactApplyButton = new BButton("compactApply", "Apply",
		new BMessage(kMsgEditApply));

	fCompactButtonGroup = new BGroupView(B_HORIZONTAL, B_USE_SMALL_SPACING);
	BLayoutBuilder::Group<>(fCompactButtonGroup)
		.SetInsets(B_USE_SMALL_SPACING, B_USE_SMALL_SPACING,
			B_USE_SMALL_SPACING, B_USE_SMALL_SPACING)
		.AddGlue()
		.Add(fCompactDiscardButton)
		.Add(fCompactApplyButton)
		.End();
	fCompactButtonGroup->Hide();

	_UpdateEditButtons();

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
		.Add(fCompactView)
		.Add(fCompactButtonGroup)
		.Add(fPreviewGroup)
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

	fViewMenu = new BMenu("View");
	fViewMenu->SetRadioMode(true);
	BMenuItem* columnListItem = new BMenuItem("ColumnListView",
		new BMessage(kMsgViewColumnList));
	fViewMenu->AddItem(columnListItem);
	fViewMenu->AddItem(new BMenuItem("CompactView",
		new BMessage(kMsgViewCompact)));
	columnListItem->SetMarked(true);
	menuBar->AddItem(fViewMenu);

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

		case kMsgFieldEditRequested:
			_HandleFieldEditRequested(message);
			break;

		case kMsgFieldEdited:
			_HandleFieldEdited(message);
			break;

		case kMsgFieldEditorClosed:
		{
			// Only forget the editor that actually closed -- a newer one
			// may already have taken its place.
			void* editor = NULL;
			if (message->FindPointer("editor", &editor) == B_OK
					&& editor == fFieldEditor) {
				fFieldEditor = NULL;
			}
			break;
		}

		case kMsgEditApply:
			_ApplyPendingEdits(true);
			break;

		case kMsgEditDiscard:
			_DiscardPendingEdits();
			break;

		case kMsgViewColumnList:
			_SetViewMode(false);
			break;

		case kMsgViewCompact:
			_SetViewMode(true);
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

		case kMsgCoverArtImageFound:
			_HandleCoverArtImageFound(message);
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
				record.album.String(), record.durationSeconds);
		} else if (!record.IsUntagged()) {
			fSearchWindow->SetQuery(record.artist.String(),
				record.title.String(), record.album.String(),
				record.durationSeconds);
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

	// Optional: only present when an album / track time was entered.
	BString album;
	message->FindString("album", &album);
	int32 durationSeconds = -1;
	if (message->FindInt32("durationSeconds", &durationSeconds) != B_OK)
		durationSeconds = -1;

	BString status("Searching MusicBrainz for \"");
	status << artist << "\" - \"" << song << "\"";
	if (album.Length() > 0)
		status << " on \"" << album << "\"";
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
	params->album = album;
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

	// Optional: only present when the search included an album / time.
	BString album;
	message->FindString("album", &album);
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
			song, album, durationSeconds);
	} else {
		// Reused window: refresh the title for this search's terms.
		fResultsWindow->SetQuery(artist, song, album, durationSeconds);
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
	// Hand edits still pending are kept first, so a later Discard Changes
	// can't undo this match along with them.
	_ApplyPendingEdits(false);

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
	if (fSearchTargetRow == fCoverArtShownRow)
		_RefreshCoverArt();

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
	fCoverArtCandidates.clear();
	fCoverArtPickerOpened = false;

	// A picker from an earlier lookup is out of date now. Quit() (unlike
	// closing it) doesn't send us the "window closed" message.
	if (fCoverArtWindow != NULL && fCoverArtWindow->Lock())
		fCoverArtWindow->Quit();
	fCoverArtWindow = NULL;

	CoverArtThreadParams* params = new CoverArtThreadParams;
	params->target = BMessenger(this);
	params->releases = releases;
	params->artist = row->Record().artist;
	params->album = row->Record().album;
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
TagViewWindow::_HandleCoverArtImageFound(BMessage* message)
{
	// A newer lookup has started since this one; ignore its stragglers.
	int32 requestId = 0;
	message->FindInt32("requestId", &requestId);
	if (requestId != fCoverArtRequestId)
		return;

	CoverArtImage image;
	BString text;
	if (message->FindString("releaseId", &text) == B_OK)
		image.releaseId = text;
	if (message->FindString("releaseTitle", &text) == B_OK)
		image.releaseTitle = text;
	message->FindInt32("releaseYear", &image.year);
	if (message->FindString("mimeType", &text) == B_OK)
		image.mimeType = text;

	const void* bytes = NULL;
	ssize_t size = 0;
	if (message->FindData("imageData", B_RAW_TYPE, &bytes, &size) != B_OK
			|| size <= 0) {
		return;
	}
	const unsigned char* begin = static_cast<const unsigned char*>(bytes);
	image.data.assign(begin, begin + size);

	fCoverArtCandidates.push_back(image);

	// The picker is already up: just add the new cover to it.
	if (fCoverArtWindow != NULL) {
		fCoverArtWindow->AddImage(image);
		return;
	}

	// Enough covers have piled up that waiting for the rest would be
	// tedious: open the picker with what we have and keep filling it.
	// (Not again if the user already closed it this lookup.)
	if (!fCoverArtPickerOpened
			&& (int32)fCoverArtCandidates.size() > kCoverArtPickerThreshold) {
		_ShowCoverArtPicker();
		_SetStatus(true, "Choose a cover -- still looking for more.");
		return;
	}

	BString status("Looking for cover art");
	status << B_UTF8_ELLIPSIS << " " << (int32)fCoverArtCandidates.size()
		<< " found";
	_SetStatus(true, status.String());
}


void
TagViewWindow::_HandleCoverArtFetched(BMessage* message)
{
	int32 requestId = 0;
	message->FindInt32("requestId", &requestId);
	if (requestId != fCoverArtRequestId)
		return;

	fCoverArtReleasesChecked = 0;
	message->FindInt32("releasesChecked", &fCoverArtReleasesChecked);

	int32 found = (int32)fCoverArtCandidates.size();

	if (found == 0) {
		BString status("No cover art found (");
		status << fCoverArtReleasesChecked << " releases and iTunes checked).";
		_SetStatus(false, status.String());
		return;
	}

	if (fCoverArtTargetRow == NULL) {
		_SetStatus(false, "Found cover art, but there's no file for it.");
		return;
	}

	BString counts;
	counts << found << (found == 1 ? " cover" : " covers") << " ("
		<< fCoverArtReleasesChecked << " releases checked)";

	// The picker was opened early and is still around: nothing left to
	// load, so drop its "still looking" note.
	if (fCoverArtWindow != NULL) {
		fCoverArtWindow->SetLoading(false);
		BString status("Found ");
		status << counts << " -- choose one.";
		_SetStatus(false, status.String());
		return;
	}

	// The user already dealt with an early picker (chose or cancelled).
	if (fCoverArtPickerOpened) {
		_SetStatus(false, "Cover art lookup finished.");
		return;
	}

	// One candidate (the common case): just use it, as Hare does.
	if (found == 1) {
		_SetCoverArt(fCoverArtTargetRow, fCoverArtCandidates[0]);

		BString status("Found ");
		status << counts << " -- using it (not saved yet).";
		_SetStatus(false, status.String());
		return;
	}

	// A few: let the user choose.
	BString status("Found ");
	status << counts << " -- choose one.";
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
	fCoverArtPickerOpened = true;
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
		fCompactView->SetRecord(NULL);
		fCompactView->SetCoverBitmap(NULL);
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

	// The compact view shows the same row (and takes its own bitmap, since
	// each view owns the one it's given).
	fCompactView->SetRecord(&record);
	fCompactView->SetCoverBitmap(haveImage ? tagkit::decode_cover_art(image)
		: NULL);
}


void
TagViewWindow::_HandleFieldEditRequested(BMessage* message)
{
	int32 fieldNumber = -1;
	if (message->FindInt32("field", &fieldNumber) != B_OK || fieldNumber < 0
			|| fieldNumber >= tagkit::TAG_FIELD_COUNT) {
		return;
	}
	tagkit::tag_field field = (tagkit::tag_field)fieldNumber;

	// The list says which row it was; the compact view always shows the
	// last selected one.
	tagkit::TagRow* row = fCoverArtShownRow;
	void* pointer = NULL;
	if (message->FindPointer("row", &pointer) == B_OK)
		row = static_cast<tagkit::TagRow*>(pointer);
	if (row == NULL) {
		_SetStatus(false, "Select a file first, then right-click a field.");
		return;
	}

	BPoint where;
	float width = 0;
	message->FindPoint("where", &where);
	message->FindFloat("width", &width);

	// Whatever comes back from the editor must find its way to this row
	// and field, even if the selection moves in the meantime.
	BMessage result(kMsgFieldEdited);
	result.AddInt32("field", fieldNumber);
	result.AddPointer("row", row);

	// An editor that's still open will accept its text and close itself
	// (clicking here deactivated it), so just start the new one.
	BString value = tagkit::tag_field_value(row->Record(), field);
	fFieldEditor = new FieldEditorWindow(BMessenger(this), result,
		tagkit::tag_field_label(field), value.String(), where, width);
	fFieldEditor->Show();
}


void
TagViewWindow::_HandleFieldEdited(BMessage* message)
{
	int32 fieldNumber = -1;
	void* pointer = NULL;
	BString value;
	if (message->FindInt32("field", &fieldNumber) != B_OK
			|| message->FindPointer("row", &pointer) != B_OK
			|| message->FindString("value", &value) != B_OK
			|| fieldNumber < 0 || fieldNumber >= tagkit::TAG_FIELD_COUNT
			|| pointer == NULL) {
		return;
	}
	tagkit::tag_field field = (tagkit::tag_field)fieldNumber;
	tagkit::TagRow* row = static_cast<tagkit::TagRow*>(pointer);

	TagRecord record = row->Record();
	if (!tagkit::set_tag_field_value(record, field, value)) {
		BString status(tagkit::tag_field_label(field));
		status << " must be blank or a whole number (up to 9999).";
		_SetStatus(false, status.String());
		return;
	}

	// Nothing actually changed.
	if (tagkit::tag_field_value(record, field)
			== tagkit::tag_field_value(row->Record(), field)) {
		return;
	}

	// The first edit to a row remembers how it was, for Discard Changes.
	if (fPendingEdits.find(row) == fPendingEdits.end())
		fPendingEdits[row] = row->Record();

	record.modified = true;
	fTagView->UpdateRow(row, record);
	if (row == fCoverArtShownRow)
		fCompactView->SetRecord(&row->Record());

	_UpdateEditButtons();

	BString status("Edited ");
	status << tagkit::tag_field_label(field)
		<< " -- Apply keeps it, Discard Changes undoes it.";
	_SetStatus(false, status.String());
}


void
TagViewWindow::_ApplyPendingEdits(bool announce)
{
	if (fPendingEdits.empty())
		return;

	int32 count = (int32)fPendingEdits.size();

	// The rows already show the edits (and are marked as modified), so
	// applying them just means no longer being able to undo them here.
	fPendingEdits.clear();
	_UpdateEditButtons();

	if (announce) {
		BString status("Applied changes to ");
		status << count << (count == 1 ? " file" : " files")
			<< " (not saved yet -- File > Save writes them).";
		_SetStatus(false, status.String());
	}
}


void
TagViewWindow::_DiscardPendingEdits()
{
	if (fPendingEdits.empty())
		return;

	int32 count = (int32)fPendingEdits.size();

	for (std::map<tagkit::TagRow*, TagRecord>::iterator it
			= fPendingEdits.begin(); it != fPendingEdits.end(); ++it) {
		fTagView->UpdateRow(it->first, it->second);
		if (it->first == fCoverArtShownRow)
			fCompactView->SetRecord(&it->first->Record());
	}
	fPendingEdits.clear();
	_UpdateEditButtons();

	BString status("Discarded changes to ");
	status << count << (count == 1 ? " file." : " files.");
	_SetStatus(false, status.String());
}


void
TagViewWindow::_UpdateEditButtons()
{
	bool pending = !fPendingEdits.empty();
	fApplyButton->SetEnabled(pending);
	fDiscardButton->SetEnabled(pending);
	fCompactApplyButton->SetEnabled(pending);
	fCompactDiscardButton->SetEnabled(pending);
}


void
TagViewWindow::_SetViewMode(bool compact)
{
	// Swap which of the two views is showing; both follow the same
	// last-selected row, so refresh the one coming up.
	if (compact) {
		if (fCompactView->IsHidden()) {
			fTagView->Hide();
			fPreviewGroup->Hide();
			fCompactView->Show();
			fCompactButtonGroup->Show();
		}
	} else {
		if (!fCompactView->IsHidden()) {
			fCompactButtonGroup->Hide();
			fCompactView->Hide();
			fTagView->Show();
			fPreviewGroup->Show();
		}
	}

	BMenuItem* item = fViewMenu->ItemAt(compact ? 1 : 0);
	if (item != NULL)
		item->SetMarked(true);

	_RefreshCoverArt();
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
	_ApplyPendingEdits(false);

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
	_ApplyPendingEdits(false);

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

	if (text != NULL) {
		// Keep the status bar to one short line; the full text stays
		// available as a tooltip.
		BString shown(text);
		if (shown.CountChars() > kMaxStatusChars) {
			shown.TruncateChars(kMaxStatusChars - 1);
			shown << B_UTF8_ELLIPSIS;
		}
		fStatusView->SetText(shown.String());
		fStatusView->SetToolTip(text);
	}
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
