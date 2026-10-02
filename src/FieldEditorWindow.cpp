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
#include <Menu.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Message.h>
#include <PopUpMenu.h>
#include <Rect.h>
#include <Screen.h>
#include <TextControl.h>
#include <TextView.h>

#include "Messages.h"


static const float kMinWidth = 280;


FieldEditorWindow::FieldEditorWindow(BMessenger target, const BMessage& result,
	const char* label, const char* initialText, BPoint screenPosition,
	float width, const std::vector<BString>* choices,
	const std::vector<BString>* recent)
	:
	BWindow(BRect(0, 0, 100, 50), "Edit", B_TITLED_WINDOW_LOOK,
		B_NORMAL_WINDOW_FEEL,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE
			| B_AUTO_UPDATE_SIZE_LIMITS | B_ASYNCHRONOUS_CONTROLS),
	fTarget(target),
	fResult(result),
	fTextControl(NULL),
	fMenuField(NULL),
	fChoice(initialText != NULL ? initialText : ""),
	fUsesChoices(choices != NULL),
	fFinished(false)
{
	BString title("Edit ");
	title << label;
	SetTitle(title.String());

	BString controlLabel(label);
	controlLabel << ":";

	if (choices != NULL) {
		BPopUpMenu* menu = new BPopUpMenu("choices");
		menu->SetLabelFromMarked(true);
		_BuildChoiceMenu(menu, *choices, recent, initialText);

		fMenuField = new BMenuField("fieldEditor", controlLabel.String(),
			menu);
		BLayoutBuilder::Group<>(this, B_VERTICAL)
			.SetInsets(B_USE_WINDOW_INSETS)
			.Add(fMenuField)
			.End();
	} else {
		fTextControl = new BTextControl("fieldEditor", controlLabel.String(),
			initialText != NULL ? initialText : "",
			new BMessage(kMsgFieldEditorCommit));
		fTextControl->SetTarget(this);

		BLayoutBuilder::Group<>(this, B_VERTICAL)
			.SetInsets(B_USE_WINDOW_INSETS)
			.Add(fTextControl)
			.End();
	}

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

	if (fTextControl != NULL) {
		fTextControl->MakeFocus(true);
		fTextControl->TextView()->SelectAll();
	}
}


void
FieldEditorWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgFieldEditorCommit:
			_Accept();
			break;

		case kMsgFieldEditorPick:
		{
			const char* value = "";
			message->FindString("value", &value);
			fChoice = value;
			_Accept();
			break;
		}

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

	if (active)
		return;

	if (fUsesChoices) {
		// Nothing was picked: leave the value alone.
		if (!fFinished) {
			fFinished = true;
			PostMessage(B_QUIT_REQUESTED);
		}
		return;
	}

	// Clicking anywhere else accepts what was typed.
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
	result.AddString("value",
		fUsesChoices ? fChoice.String() : fTextControl->Text());
	fTarget.SendMessage(&result);

	PostMessage(B_QUIT_REQUESTED);
}


void
FieldEditorWindow::_BuildChoiceMenu(BMenu* menu,
	const std::vector<BString>& choices, const std::vector<BString>* recent,
	const char* initialText)
{
	BString current(initialText != NULL ? initialText : "");
	bool marked = false;

	BMessage* noneMessage = new BMessage(kMsgFieldEditorPick);
	noneMessage->AddString("value", "");
	BMenuItem* none = new BMenuItem("(none)", noneMessage);
	menu->AddItem(none);
	if (current.Length() == 0) {
		none->SetMarked(true);
		marked = true;
	}
	menu->AddSeparatorItem();

	// A current value that isn't in the list stays selectable.
	bool inList = false;
	for (size_t i = 0; i < choices.size(); i++) {
		if (choices[i].ICompare(current) == 0) {
			inList = true;
			break;
		}
	}
	if (!marked && !inList) {
		BMessage* message = new BMessage(kMsgFieldEditorPick);
		message->AddString("value", current.String());
		BMenuItem* item = new BMenuItem(current.String(), message);
		item->SetMarked(true);
		menu->AddItem(item);
		menu->AddSeparatorItem();
		marked = true;
	}

	// The recent picks, newest first, then a line before the full list.
	if (recent != NULL && !recent->empty()) {
		for (size_t i = 0; i < recent->size(); i++) {
			BMessage* message = new BMessage(kMsgFieldEditorPick);
			message->AddString("value", (*recent)[i].String());
			BMenuItem* item = new BMenuItem((*recent)[i].String(), message);
			if (!marked && (*recent)[i].ICompare(current) == 0) {
				item->SetMarked(true);
				marked = true;
			}
			menu->AddItem(item);
		}
		menu->AddSeparatorItem();
	}

	for (size_t i = 0; i < choices.size(); i++) {
		BMessage* message = new BMessage(kMsgFieldEditorPick);
		message->AddString("value", choices[i].String());
		BMenuItem* item = new BMenuItem(choices[i].String(), message);
		if (!marked && choices[i].ICompare(current) == 0) {
			item->SetMarked(true);
			marked = true;
		}
		menu->AddItem(item);
	}

	menu->SetTargetForItems(this);
}
