// Messages.h
//
// Shared BMessage `what` constants for the TagView app (not part of
// tagkit -- these are app-specific wiring, not reusable widget code).

#ifndef TAGVIEW_MESSAGES_H
#define TAGVIEW_MESSAGES_H

enum {
	kMsgFileOpen			= 'fOpn',	// File > Open... selected
	kMsgEditSearch			= 'eSrc',	// Edit > Search... selected
	kMsgHelpAbout			= 'hAbt',	// Help > About TagView selected

	kMsgSearchTextChanged	= 'sTxC',	// Artist or Song field edited
	kMsgSearchRequested		= 'sReq',	// Search button pressed
	kMsgSearchWindowClosed	= 'sWCl',	// SearchWindow is going away
	kMsgSearchCompleted		= 'sCmp'	// background search thread is done
};

#endif // TAGVIEW_MESSAGES_H
