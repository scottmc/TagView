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

#include <map>
#include <vector>

#include <String.h>
#include <Window.h>

#include "tagkit/CoverArtImage.h"
#include "tagkit/RecordingMatch.h"
#include "tagkit/TagRecord.h"

class BButton;
class BFilePanel;
class BGroupView;
class BMenu;
class BMenuBar;
class BMessage;
class BStringView;
class Barberpole;

namespace tagkit {
	class CompactView;
	class CoverArtView;
	class TagView;
	class TagRow;
}

class FieldEditorWindow;
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
			void				_HandleReleaseSearchCompleted(
									BMessage* message);
			void				_HandleApplyMatch(BMessage* message);
			void				_StartCoverArtFetch(tagkit::TagRow* row,
									const std::vector<tagkit::ReleaseRef>&
										releases,
									const char* statusText,
									const char* artist = NULL,
									const char* album = NULL);
									// artist/album: what iTunes is asked
									// for; NULL = the row's own tags
			void				_HandleCoverArtImageFound(BMessage* message);
			void				_HandleCoverArtFetched(BMessage* message);
			void				_HandleCoverArtChosen(BMessage* message);
			void				_SetCoverArt(tagkit::TagRow* row,
									const tagkit::CoverArtImage& image);
			void				_HandleSelectionChanged();

	// Hand-editing a tag field (right-click on it in either view). Edits
	// show in the views at once but stay "pending" until Apply (kept, as
	// unsaved changes to the row -- File > Save writes them) or Discard
	// Changes (put back as they were).
			void				_HandleFieldEditRequested(BMessage* message);
			void				_HandleFieldEdited(BMessage* message);
			void				_ApplyPendingEdits(bool announce);
			void				_DiscardPendingEdits();
			void				_UpdateEditButtons();
			void				_SetViewMode(bool compact);
	static	float				_CompactScale(int32 size);
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
			BGroupView*			fPreviewGroup;	// holds fCoverArtView

			// The alternative to fTagView + fPreviewGroup (View menu);
			// shows the same last-selected row.
			tagkit::CompactView*	fCompactView;
			BMenu*				fViewMenu;
			BMenu*				fCompactSizeMenu;
			tagkit::TagRow*		fCoverArtShownRow;

			// Apply / Discard Changes under the list, and the same pair
			// under the compact view (only one of the two is showing).
			// Enabled only while fPendingEdits isn't empty.
			BButton*			fDiscardButton;
			BButton*			fApplyButton;
			BGroupView*			fCompactButtonGroup;
			BButton*			fCompactDiscardButton;
			BButton*			fCompactApplyButton;

			// Rows with hand edits not yet applied, each with its record as
			// it was before the first of those edits.
			std::map<tagkit::TagRow*, tagkit::TagRecord>	fPendingEdits;
			FieldEditorWindow*	fFieldEditor;

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
