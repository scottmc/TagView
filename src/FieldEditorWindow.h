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

#include <Messenger.h>
#include <String.h>
#include <Window.h>

class BTextControl;


// A small window with one text field, opened where the user
// right-clicked a tag field (in either view) to edit its value. Return
// (or clicking anywhere else) accepts the text; Escape cancels.
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
									BPoint screenPosition, float width);

	virtual	void				MessageReceived(BMessage* message);
	virtual	void				DispatchMessage(BMessage* message,
									BHandler* handler);
	virtual	void				WindowActivated(bool active);
	virtual	bool				QuitRequested();

private:
			void				_Accept();

			BMessenger			fTarget;
			BMessage			fResult;
			BTextControl*		fTextControl;
			bool				fFinished;	// accepted or cancelled already
};

#endif // TAGVIEW_FIELD_EDITOR_WINDOW_H
