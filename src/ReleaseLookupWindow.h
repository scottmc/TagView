/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
#ifndef TAGVIEW_RELEASE_LOOKUP_WINDOW_H
#define TAGVIEW_RELEASE_LOOKUP_WINDOW_H

#include <Window.h>

class BButton;
class BTextControl;
class BTextView;


// A small test window for Edit > Look Up Release ID...: type or paste a
// MusicBrainz disc ID (what Hare prints as "disc ID" when it reads a CD) or a
// release ID (the long one with dashes, in a release's web address) and press
// Look Up. It asks tagkit::MusicBrainzSearch::LookupDiscId() or
// LookupRelease() on its own thread and lists the album's details and the
// track list it got back. It exists to try those two lookups, which Hare
// will use, without a CD; it changes nothing in TagView's file list.
class ReleaseLookupWindow : public BWindow {
public:
								ReleaseLookupWindow();

	virtual	void				MessageReceived(BMessage* message);

private:
			void				_StartLookup();
			void				_ShowText(const char* text);

			BTextControl*		fIdControl;
			BButton*			fLookUpButton;
			BTextView*			fResultView;
			bool				fLookingUp;
};

#endif // TAGVIEW_RELEASE_LOOKUP_WINDOW_H
