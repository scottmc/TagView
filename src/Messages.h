/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
// Messages.h
//
// Shared BMessage `what` constants for the TagView app (not part of
// tagkit -- these are app-specific wiring, not reusable widget code).

#ifndef TAGVIEW_MESSAGES_H
#define TAGVIEW_MESSAGES_H

enum {
	kMsgFileOpen			= 'fOpn',	// File > Open... selected
	kMsgFileSave			= 'fSav',	// File > Save selected
	kMsgFileSaveAll			= 'fSvA',	// File > Save All selected
	kMsgEditSearch			= 'eSrc',	// Edit > Search... selected
	kMsgEditChooseCoverArt	= 'eCAr',	// Edit > Choose Cover Art... selected
	kMsgSelectionChanged	= 'sSel',	// the list's selection changed
	kMsgViewColumnList		= 'vCol',	// View > ColumnListView selected
	kMsgViewCompact			= 'vCmp',	// View > CompactView selected
	kMsgEditApply			= 'eApl',	// Apply (pending hand edits) pressed
	kMsgEditDiscard			= 'eDsc',	// Discard Changes pressed
	kMsgHelpAbout			= 'hAbt',	// Help > About TagView selected

	kMsgFieldEditRequested	= 'fEdR',	// a view: right-click on an editable field
	kMsgFieldEdited			= 'fEdt',	// FieldEditorWindow accepted new text
	kMsgFieldEditorCommit	= 'fECm',	// (inside FieldEditorWindow) Return pressed
	kMsgFieldEditorPick		= 'fEPk',	// (inside FieldEditorWindow) menu entry chosen
	kMsgFieldEditorClosed	= 'fECl',	// FieldEditorWindow is going away

	kMsgSearchTextChanged	= 'sTxC',	// Artist, Song or Time field edited
	kMsgSearchRequested		= 'sReq',	// Search button pressed
	kMsgSearchWindowClosed	= 'sWCl',	// SearchWindow is going away
	kMsgSearchCompleted		= 'sCmp',	// background search thread is done

	kMsgApplyMatch			= 'aMat',	// user picked a MusicBrainz result
	kMsgResultsWindowClosed	= 'rWCl',	// SearchResultsWindow is going away

	kMsgCoverArtImageFound	= 'cAIm',	// lookup found one cover (more may follow)
	kMsgCoverArtFetched		= 'cAFd',	// background cover art lookup is done
	kMsgCoverArtSelectionChanged = 'cASc',	// picker: another thumbnail picked
	kMsgCoverArtChosen		= 'cACh',	// user picked a cover ("index")
	kMsgCoverArtWindowClosed = 'cAWc'	// CoverArtPickerWindow is going away
};

#endif // TAGVIEW_MESSAGES_H
