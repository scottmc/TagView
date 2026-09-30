/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
#ifndef TAGVIEW_SEARCH_WINDOW_H
#define TAGVIEW_SEARCH_WINDOW_H

#include <Messenger.h>
#include <Window.h>

class BButton;
class BTextControl;


// A small, non-modal dialog for Edit > Search...: Artist + Song text
// fields, an optional track Time field (m:ss, or plain seconds), and a
// Search button that stays disabled until Artist and Song are filled in
// (and Time, if given, is a valid time). Pressing Search sends a
// kMsgSearchRequested message (with "artist" and "song" strings, plus an
// int32 "durationSeconds" only when a time was entered) to whatever
// BMessenger was given at construction -- TagViewWindow, which runs the
// actual MusicBrainz lookup.
class SearchWindow : public BWindow {
public:
	explicit					SearchWindow(BMessenger target);

	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();

			void				MoveToFrontAndFocus();

	// Pre-fills the Artist/Song/Time fields (e.g. from a guess based on
	// the selected file's name and its length) and updates the Search
	// button's enabled state to match. durationSeconds < 0 leaves Time
	// blank.
			void				SetQuery(const char* artist,
									const char* song,
									int32 durationSeconds = -1);

private:
			void				_UpdateSearchButtonEnabled();

			BMessenger			fTarget;
			BTextControl*		fArtistControl;
			BTextControl*		fSongControl;
			BTextControl*		fTimeControl;
			BButton*			fSearchButton;
};

#endif // TAGVIEW_SEARCH_WINDOW_H
