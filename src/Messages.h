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
	kMsgHelpAbout			= 'hAbt',	// Help > About TagView selected

	kMsgSearchTextChanged	= 'sTxC',	// Artist, Song or Time field edited
	kMsgSearchRequested		= 'sReq',	// Search button pressed
	kMsgSearchWindowClosed	= 'sWCl',	// SearchWindow is going away
	kMsgSearchCompleted		= 'sCmp',	// background search thread is done

	kMsgApplyMatch			= 'aMat',	// user picked a MusicBrainz result
	kMsgResultsWindowClosed	= 'rWCl',	// SearchResultsWindow is going away

	kMsgCoverArtFetched		= 'cAFd',	// background cover art lookup is done
	kMsgCoverArtSelectionChanged = 'cASc',	// picker: another thumbnail picked
	kMsgCoverArtChosen		= 'cACh',	// user picked a cover ("index")
	kMsgCoverArtWindowClosed = 'cAWc'	// CoverArtPickerWindow is going away
};

#endif // TAGVIEW_MESSAGES_H
