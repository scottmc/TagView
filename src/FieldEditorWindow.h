/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
#ifndef TAGVIEW_FIELD_EDITOR_WINDOW_H
#define TAGVIEW_FIELD_EDITOR_WINDOW_H

#include <vector>

#include <Messenger.h>
#include <String.h>
#include <Window.h>

class BMenu;
class BMenuField;
class BTextControl;


// A small window with one text field, opened where the user
// right-clicked a tag field (in either view) to edit its value. Return
// (or clicking anywhere else) accepts the text; Escape cancels.
//
// With a list of choices (the genre list, say) the text field is replaced
// by a drop-down menu of them: picking one accepts it straight away.
// Recently picked choices (if given) come first, set off from the full list
// by a separator line. A "(none)" entry at the top clears the value, and a current value that
// isn't in the list is kept as an entry of its own so opening the editor
// never loses it.
//
// Accepting sends a copy of the message given at construction to the
// target, with the text added as "value" -- the caller puts whatever it
// needs to find its way back (which row, which field) in that message.
// Cancelling sends nothing. Either way, once the window is gone it sends
// kMsgFieldEditorClosed (with this window as "editor") so the caller can
// drop its pointer.
class FieldEditorWindow : public BWindow {
public:
								FieldEditorWindow(BMessenger target,
									const BMessage& result,
									const char* label,
									const char* initialText,
									BPoint screenPosition, float width,
									const std::vector<BString>* choices
										= NULL);

	virtual	void				MessageReceived(BMessage* message);
	virtual	void				DispatchMessage(BMessage* message,
									BHandler* handler);
	virtual	void				WindowActivated(bool active);
	virtual	bool				QuitRequested();

private:
			void				_Accept();
			void				_BuildChoiceMenu(BMenu* menu,
									const std::vector<BString>& choices,
									const std::vector<BString>* recent,
									const char* initialText);

			BMessenger			fTarget;
			BMessage			fResult;
			BTextControl*		fTextControl;
			BMenuField*			fMenuField;
			BString				fChoice;	// drop-down mode: current pick
			bool				fUsesChoices;
			bool				fFinished;	// accepted or cancelled already
};

#endif // TAGVIEW_FIELD_EDITOR_WINDOW_H
