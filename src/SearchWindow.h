#ifndef TAGVIEW_SEARCH_WINDOW_H
#define TAGVIEW_SEARCH_WINDOW_H

#include <Messenger.h>
#include <Window.h>

class BButton;
class BTextControl;


// A small, non-modal dialog for Edit > Search...: Artist + Song text
// fields and a Search button that stays disabled until both are filled
// in. Pressing Search sends a kMsgSearchRequested message (with "artist"
// and "song" strings) to whatever BMessenger was given at construction
// -- TagViewWindow, which runs the actual MusicBrainz lookup.
class SearchWindow : public BWindow {
public:
	explicit					SearchWindow(BMessenger target);

	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();

			void				MoveToFrontAndFocus();

	// Pre-fills the Artist/Song fields (e.g. from a guess based on the
	// selected file's name) and updates the Search button's enabled
	// state to match.
			void				SetQuery(const char* artist,
									const char* song);

private:
			void				_UpdateSearchButtonEnabled();

			BMessenger			fTarget;
			BTextControl*		fArtistControl;
			BTextControl*		fSongControl;
			BButton*			fSearchButton;
};

#endif // TAGVIEW_SEARCH_WINDOW_H
