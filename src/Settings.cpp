/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
#include "Settings.h"

#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <Path.h>
#include <Screen.h>
#include <Window.h>


static const char* kSettingsFileName = "TagView";


Settings&
Settings::Get()
{
	static Settings settings;
	return settings;
}


Settings::Settings()
	:
	fCompactSize(1)
{
	_Load();
}


void
Settings::RestoreWindow(const char* name, BWindow* window) const
{
	BRect frame;
	if (window == NULL || fWindows.FindRect(name, &frame) != B_OK
			|| !frame.IsValid()) {
		return;
	}

	// Only if some of it would still be on a screen (monitors come and go).
	BRect visible = BScreen(window).Frame();
	BRect titleStrip(frame.left, frame.top, frame.right, frame.top + 20);
	if (!visible.Intersects(titleStrip))
		return;

	window->MoveTo(frame.LeftTop());
	if ((window->Flags() & B_NOT_RESIZABLE) == 0)
		window->ResizeTo(frame.Width(), frame.Height());
}


void
Settings::SaveWindow(const char* name, BWindow* window)
{
	if (window == NULL)
		return;

	fWindows.RemoveName(name);
	fWindows.AddRect(name, window->Frame());
	_Save();
}


int32
Settings::CompactSize() const
{
	return fCompactSize;
}


void
Settings::SetCompactSize(int32 size)
{
	if (size < 1 || size > 3 || size == fCompactSize)
		return;

	fCompactSize = size;
	_Save();
}


void
Settings::AddRecentGenre(const char* genre)
{
	if (genre == NULL || genre[0] == '\0')
		return;

	// Already in the list: take it out so it can go to the front.
	for (size_t i = 0; i < fRecentGenres.size(); i++) {
		if (fRecentGenres[i].ICompare(genre) == 0) {
			fRecentGenres.erase(fRecentGenres.begin() + i);
			break;
		}
	}

	fRecentGenres.insert(fRecentGenres.begin(), BString(genre));
	if ((int32)fRecentGenres.size() > kMaxRecentGenres)
		fRecentGenres.resize(kMaxRecentGenres);

	_Save();
}


void
Settings::_Load()
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return;
	path.Append(kSettingsFileName);

	BFile file(path.Path(), B_READ_ONLY);
	if (file.InitCheck() != B_OK)
		return;

	BMessage archive;
	if (archive.Unflatten(&file) != B_OK)
		return;

	archive.FindMessage("windows", &fWindows);

	int32 size;
	if (archive.FindInt32("compactSize", &size) == B_OK && size >= 1
			&& size <= 3) {
		fCompactSize = size;
	}

	BString genre;
	for (int32 i = 0; archive.FindString("recentGenre", i, &genre) == B_OK
			&& i < kMaxRecentGenres; i++) {
		fRecentGenres.push_back(genre);
	}
}


void
Settings::_Save() const
{
	BMessage archive;
	archive.AddMessage("windows", &fWindows);
	archive.AddInt32("compactSize", fCompactSize);
	for (size_t i = 0; i < fRecentGenres.size(); i++)
		archive.AddString("recentGenre", fRecentGenres[i]);

	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path, true) != B_OK)
		return;
	path.Append(kSettingsFileName);

	BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if (file.InitCheck() == B_OK)
		archive.Flatten(&file);
}
