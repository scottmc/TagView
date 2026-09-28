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
}

class SearchWindow;


class TagViewWindow : public BWindow {
public:
								TagViewWindow();
	virtual						~TagViewWindow();

	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();

private:
			BMenuBar*			_BuildMenuBar();
			void				_AddRefs(BMessage* message);
			void				_HandleSearchRequested(BMessage* message);
			void				_HandleSearchCompleted(BMessage* message);
			void				_ShowAbout();
			void				_SetStatus(bool busy, const char* text);

			tagkit::TagView*	fTagView;
			Barberpole*			fBusyIndicator;
			BStringView*		fStatusView;
			BFilePanel*			fOpenPanel;
			SearchWindow*		fSearchWindow;
};

#endif // TAGVIEW_WINDOW_H
