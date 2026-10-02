/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
#include "FieldEditorWindow.h"

#include <LayoutBuilder.h>
#include <Message.h>
#include <Rect.h>
#include <Screen.h>
#include <TextControl.h>
#include <TextView.h>

#include "Messages.h"


static const float kMinWidth = 280;


FieldEditorWindow::FieldEditorWindow(BMessenger target, const BMessage& result,
	const char* label, const char* initialText, BPoint screenPosition,
	float width)
	:
	BWindow(BRect(0, 0, 100, 50), "Edit", B_TITLED_WINDOW_LOOK,
		B_NORMAL_WINDOW_FEEL,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE
			| B_AUTO_UPDATE_SIZE_LIMITS | B_ASYNCHRONOUS_CONTROLS),
	fTarget(target),
	fResult(result),
	fTextControl(NULL),
	fFinished(false)
{
	BString title("Edit ");
	title << label;
	SetTitle(title.String());

	BString controlLabel(label);
	controlLabel << ":";
	fTextControl = new BTextControl("fieldEditor", controlLabel.String(),
		initialText != NULL ? initialText : "",
		new BMessage(kMsgFieldEditorCommit));
	fTextControl->SetTarget(this);

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_WINDOW_INSETS)
		.Add(fTextControl)
		.End();

	// Wide enough for the cell it replaces (but never cramped), placed
	// with the text field around the pointer and kept on screen.
	float wanted = width > kMinWidth ? width : kMinWidth;
	ResizeTo(wanted, Bounds().Height());
	ResizeToPreferred();
	if (Bounds().Width() < wanted)
		ResizeTo(wanted, Bounds().Height());

	BRect screenFrame = BScreen(this).Frame();
	BPoint position(screenPosition.x - 24, screenPosition.y - Bounds().Height()
		/ 2.0f - 8);
	if (position.x + Bounds().Width() > screenFrame.right)
		position.x = screenFrame.right - Bounds().Width();
	if (position.y + Bounds().Height() > screenFrame.bottom)
		position.y = screenFrame.bottom - Bounds().Height();
	if (position.x < screenFrame.left)
		position.x = screenFrame.left;
	if (position.y < screenFrame.top)
		position.y = screenFrame.top;
	MoveTo(position);

	fTextControl->MakeFocus(true);
	fTextControl->TextView()->SelectAll();
}


void
FieldEditorWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgFieldEditorCommit:
			_Accept();
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void
FieldEditorWindow::DispatchMessage(BMessage* message, BHandler* handler)
{
	// Escape cancels. (The text field would otherwise just swallow it.)
	if (message->what == B_KEY_DOWN) {
		int8 byte = 0;
		if (message->FindInt8("byte", &byte) == B_OK && byte == B_ESCAPE) {
			fFinished = true;
			PostMessage(B_QUIT_REQUESTED);
			return;
		}
	}

	BWindow::DispatchMessage(message, handler);
}


void
FieldEditorWindow::WindowActivated(bool active)
{
	BWindow::WindowActivated(active);

	// Clicking anywhere else accepts what was typed.
	if (!active)
		_Accept();
}


bool
FieldEditorWindow::QuitRequested()
{
	BMessage closed(kMsgFieldEditorClosed);
	closed.AddPointer("editor", this);
	fTarget.SendMessage(&closed);
	return true;
}


void
FieldEditorWindow::_Accept()
{
	if (fFinished)
		return;
	fFinished = true;

	BMessage result(fResult);
	result.AddString("value", fTextControl->Text());
	fTarget.SendMessage(&result);

	PostMessage(B_QUIT_REQUESTED);
}
