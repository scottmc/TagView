#include "SearchWindow.h"

#include <Button.h>
#include <LayoutBuilder.h>
#include <Message.h>
#include <TextControl.h>

#include "Messages.h"


static const float kWindowWidth = 320;


SearchWindow::SearchWindow(BMessenger target)
	:
	BWindow(BRect(120, 120, 120 + kWindowWidth, 220), "Search MusicBrainz",
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

	fSearchButton = new BButton("search", "Search",
		new BMessage(kMsgSearchRequested));
	fSearchButton->SetEnabled(false);

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_WINDOW_INSETS)
		.Add(fArtistControl)
		.Add(fSongControl)
		.AddGlue()
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(fSearchButton)
		.End()
		.End();

	fArtistControl->SetTarget(this);
	fSongControl->SetTarget(this);
	fSearchButton->SetTarget(this);

	fArtistControl->MakeFocus(true);
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
			fTarget.SendMessage(&request);
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
	fTarget.SendMessage(kMsgSearchWindowClosed);
	return true;
}


void
SearchWindow::SetQuery(const char* artist, const char* song)
{
	fArtistControl->SetText(artist != NULL ? artist : "");
	fSongControl->SetText(song != NULL ? song : "");
	_UpdateSearchButtonEnabled();
}


void
SearchWindow::_UpdateSearchButtonEnabled()
{
	bool ready = fArtistControl->TextLength() > 0
		&& fSongControl->TextLength() > 0;
	fSearchButton->SetEnabled(ready);
}


void
SearchWindow::MoveToFrontAndFocus()
{
	if (IsHidden())
		Show();
	Activate();
}
