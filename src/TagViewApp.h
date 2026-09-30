/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef TAGVIEW_APP_H
#define TAGVIEW_APP_H

#include <Application.h>

class TagViewWindow;


class TagViewApp : public BApplication {
public:
								TagViewApp();

	virtual	void				ReadyToRun();
	virtual	void				RefsReceived(BMessage* message);

private:
			TagViewWindow*		fWindow;
};

#endif // TAGVIEW_APP_H
