/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
#ifndef TAGVIEW_SEARCH_RESULTS_WINDOW_H
#define TAGVIEW_SEARCH_RESULTS_WINDOW_H

#include <Messenger.h>
#include <String.h>
#include <Window.h>
#include <vector>

#include "tagkit/RecordingMatch.h"

class BButton;

namespace tagkit {
	class RecordingMatchView;
}


// Shows the candidate MusicBrainz recordings a search turned up and lets
// the user pick which one actually matches. Non-modal, mirrors
// SearchWindow's shape: picking a result (Apply, or double-clicking a
// row) sends a kMsgApplyMatch message (with the match's fields) to
// whatever BMessenger was given at construction, then closes.
class SearchResultsWindow : public BWindow {
public:
								SearchResultsWindow(BMessenger target,
									const BString& artist,
									const BString& song,
									const BString& album,
									int32 durationSeconds = -1);

	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();

	// Sets the title to show the search terms these results are for
	// (album empty == no album was part of the search; durationSeconds < 0
	// == no track time was).
			void				SetQuery(const BString& artist,
									const BString& song,
									const BString& album,
									int32 durationSeconds = -1);
			void				SetMatches(
									const std::vector<tagkit::RecordingMatch>&
										matches);
			void				MoveToFrontAndFocus();

private:
			void				_Apply();

			BMessenger					fTarget;
			tagkit::RecordingMatchView*	fMatchView;
			BButton*					fApplyButton;
};

#endif // TAGVIEW_SEARCH_RESULTS_WINDOW_H
