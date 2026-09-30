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

#include <Window.h>

class BFilePanel;
class BMenuBar;
class BMessage;
class BStringView;
class Barberpole;

namespace tagkit {
	class TagView;
	class TagRow;
}

class SearchWindow;
class SearchResultsWindow;


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
			void				_ShowAbout();
			void				_SetStatus(bool busy, const char* text);

			tagkit::TagView*	fTagView;
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
};

#endif // TAGVIEW_WINDOW_H
