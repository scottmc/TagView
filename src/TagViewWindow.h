/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
#ifndef TAGVIEW_WINDOW_H
#define TAGVIEW_WINDOW_H

#include <vector>

#include <String.h>
#include <Window.h>

#include "tagkit/CoverArtImage.h"
#include "tagkit/RecordingMatch.h"

class BFilePanel;
class BMenuBar;
class BMessage;
class BStringView;
class Barberpole;

namespace tagkit {
	class CoverArtView;
	class TagView;
	class TagRow;
}

class SearchWindow;
class SearchResultsWindow;
class CoverArtPickerWindow;


class TagViewWindow : public BWindow {
public:
								TagViewWindow();
	virtual						~TagViewWindow();

	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();

private:
			BMenuBar*			_BuildMenuBar();
			void				_AddRefs(BMessage* message);
			void				_HandleEditSearch();
			void				_HandleSearchRequested(BMessage* message);
			void				_HandleSearchCompleted(BMessage* message);
			void				_HandleApplyMatch(BMessage* message);
			void				_StartCoverArtFetch(tagkit::TagRow* row,
									const std::vector<tagkit::ReleaseRef>&
										releases,
									const char* statusText);
			void				_HandleCoverArtImageFound(BMessage* message);
			void				_HandleCoverArtFetched(BMessage* message);
			void				_HandleCoverArtChosen(BMessage* message);
			void				_SetCoverArt(tagkit::TagRow* row,
									const tagkit::CoverArtImage& image);
			void				_HandleSelectionChanged();
			void				_HandleChooseCoverArt();
			void				_ShowCoverArtPicker();
			void				_RefreshCoverArt();
			void				_HandleSave();
			void				_HandleSaveAll();

	// Writes one row's tags to its file and refreshes the row from what
	// was actually saved. Returns false (with a reason in error) if the
	// write failed, leaving the row marked as modified.
			bool				_SaveRow(tagkit::TagRow* row, BString& error);
			int32				_CountModifiedRows() const;
			void				_ShowAbout();
			void				_SetStatus(bool busy, const char* text);

			tagkit::TagView*	fTagView;

			// Shows the cover art of the last row selected in fTagView
			// (kept when the selection is cleared), and which row that is.
			tagkit::CoverArtView*	fCoverArtView;
			tagkit::TagRow*		fCoverArtShownRow;
			Barberpole*			fBusyIndicator;
			BStringView*		fStatusView;
			BFilePanel*			fOpenPanel;
			SearchWindow*		fSearchWindow;
			SearchResultsWindow*	fResultsWindow;

			// The row Edit > Search... was invoked for (whatever was
			// selected in fTagView at the time), so a later "apply this
			// match" from the results window knows which row to update.
			// NULL if nothing was selected.
			tagkit::TagRow*		fSearchTargetRow;

			// Cover art lookup state: the row it's for, a counter so a late
			// answer to an older lookup can be ignored, and the candidates
			// the picker window is currently offering.
			CoverArtPickerWindow*	fCoverArtWindow;
			tagkit::TagRow*		fCoverArtTargetRow;
			int32				fCoverArtRequestId;
			int32				fCoverArtReleasesChecked;
			bool				fCoverArtPickerOpened;	// opened for this
									// lookup (so a closed one stays closed)
			std::vector<tagkit::CoverArtImage>	fCoverArtCandidates;
};

#endif // TAGVIEW_WINDOW_H
