/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
#ifndef TAGVIEW_SETTINGS_H
#define TAGVIEW_SETTINGS_H

#include <vector>

#include <Message.h>
#include <Rect.h>
#include <String.h>

class BWindow;


// TagView's saved settings, kept as a flattened BMessage in
// ~/config/settings/TagView (the same approach Hare takes): where each
// window was and how big, the compact view's text size, and the genres
// picked most recently. Loaded when the app starts; every change is
// written straight out, so a crash doesn't lose it.
class Settings {
public:
	// The one instance, loaded on first use.
	static	Settings&			Get();

	// Window position and size, by a short name ("main", "results"...).
	// RestoreWindow() moves/resizes the window to where it was (a window
	// that can't be resized keeps its size, and one that would end up off
	// screen keeps its default place); SaveWindow() records where it is.
			void				RestoreWindow(const char* name,
									BWindow* window) const;
			void				SaveWindow(const char* name, BWindow* window);

	// CompactView's text size: 1, 2 or 3.
			int32				CompactSize() const;
			void				SetCompactSize(int32 size);

	// The genres picked in the genre editor, newest first, at most
	// kMaxRecentGenres of them. Picking one that's already there moves it
	// to the front.
	static	const int32			kMaxRecentGenres = 8;
	const std::vector<BString>&	RecentGenres() const { return fRecentGenres; }
			void				AddRecentGenre(const char* genre);

private:
								Settings();

			void				_Load();
			void				_Save() const;

			BMessage			fWindows;		// name -> BRect
			int32				fCompactSize;
			std::vector<BString> fRecentGenres;
};

#endif // TAGVIEW_SETTINGS_H
