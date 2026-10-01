/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
#ifndef TAGVIEW_COVER_ART_PICKER_WINDOW_H
#define TAGVIEW_COVER_ART_PICKER_WINDOW_H

#include <vector>

#include <Messenger.h>
#include <String.h>
#include <Window.h>

#include "tagkit/CoverArtImage.h"

class BButton;
class BStringView;

namespace tagkit {
	class CoverArtCandidatesView;
}


// Shown when MusicBrainz turned up cover art on more than one release: a
// scrollable strip of thumbnails (like Hare's cover art picker) with the
// selected release's title and year underneath. Non-modal. "Use Selected
// Cover" (or double-clicking a thumbnail) sends kMsgCoverArtChosen with
// the chosen candidate's int32 "index" -- its position in the vector given
// at construction -- to the BMessenger given, then closes.
class CoverArtPickerWindow : public BWindow {
public:
								CoverArtPickerWindow(BMessenger target,
									const BString& fileName,
									const std::vector<tagkit::CoverArtImage>&
										images);

	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();

			void				MoveToFrontAndFocus();

private:
			void				_UpdateCaption();
			void				_Choose();

			BMessenger			fTarget;
			tagkit::CoverArtCandidatesView*	fCandidatesView;
			BStringView*		fCaptionView;
			std::vector<BString>	fCaptions;
};

#endif // TAGVIEW_COVER_ART_PICKER_WINDOW_H
