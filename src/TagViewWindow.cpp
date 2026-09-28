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

#include "Messages.h"
#include "SearchWindow.h"
#include "tagkit/TagRecord.h"
#include "tagkit/TagView.h"
#include "widgetkit/Barberpole.h"

using tagkit::TagRecord;


namespace {

// Stands in for a real MusicBrainz lookup for now: runs on its own
// thread (the way the real lookup will need to, since it's a network
// call) and just reports back what it "searched" for after a short
// delay. This exists to prove out the busy-indicator/status-bar
// plumbing before the real lookup is wired in.
struct SearchThreadParams {
	BMessenger	target;
	BString		artist;
	BString		song;
};


status_t
SearchThreadEntry(void* data)
{
	SearchThreadParams* params = static_cast<SearchThreadParams*>(data);

	snooze(1500000); // 1.5s -- simulated network latency

	BMessage result(kMsgSearchCompleted);
	result.AddString("artist", params->artist);
	result.AddString("song", params->song);
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
	fSearchWindow(NULL)
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
			if (fSearchWindow == NULL) {
				fSearchWindow = new SearchWindow(BMessenger(this));
				fSearchWindow->Show();
			} else {
				fSearchWindow->MoveToFrontAndFocus();
			}
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
TagViewWindow::_HandleSearchRequested(BMessage* message)
{
	BString artist, song;
	message->FindString("artist", &artist);
	message->FindString("song", &song);

	BString status("Searching MusicBrainz for \"");
	status << artist << "\" - \"" << song << "\"" B_UTF8_ELLIPSIS;
	_SetStatus(true, status.String());

	// TODO: replace this stand-in with a real MusicBrainz recording
	// search (and present candidate matches for the user to pick from).
	// It already runs off the main thread, the way the real lookup will
	// need to, so wiring in the actual network call is a drop-in swap
	// for SearchThreadEntry()'s body.
	SearchThreadParams* params = new SearchThreadParams;
	params->target = BMessenger(this);
	params->artist = artist;
	params->song = song;

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

	BString status("MusicBrainz lookup for \"");
	status << artist << "\" - \"" << song
		<< "\" isn't wired up yet -- next step.";
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
		"\"tagkit\" library (tag data model + a BColumnListView-based "
		"tag list widget) intended to be shared with other apps such "
		"as Hare and ArmyKnife.\n\n"
		"Tag reading (TagLib), MusicBrainz lookups and cover art "
		"(libcoverart) are on the way.",
		"OK");
	alert->Go();
}
