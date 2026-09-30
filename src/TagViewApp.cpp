/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include "TagViewApp.h"

#include "TagViewWindow.h"


static const char* kAppSignature = "application/x-vnd.scottmc-TagView";


TagViewApp::TagViewApp()
	:
	BApplication(kAppSignature),
	fWindow(NULL)
{
}


void
TagViewApp::ReadyToRun()
{
	fWindow = new TagViewWindow();
	fWindow->Show();
}


void
TagViewApp::RefsReceived(BMessage* message)
{
	// Files dropped on the app icon, or opened via Tracker's "Open With",
	// arrive here before (or instead of) the window. Forward them along.
	if (fWindow != NULL)
		fWindow->PostMessage(message);
}
