/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
// CoverArtView.h
//
// Part of "tagkit" -- the reusable pieces of TagView.
// A small square view that shows one cover art bitmap, scaled to fit with
// its aspect ratio kept, or a plain "No cover art" placeholder when there
// isn't one. A simpler cousin of Hare's CoverArtView (no CD button), meant
// to sit under a tag list showing the art of the selected file.

#ifndef TAGKIT_COVER_ART_VIEW_H
#define TAGKIT_COVER_ART_VIEW_H

#include <View.h>

class BBitmap;


namespace tagkit {

class CoverArtView : public BView {
public:
								CoverArtView(const char* name,
									float size = 112.0f);
	virtual						~CoverArtView();

	virtual	void				Draw(BRect updateRect);

	// Takes ownership of bitmap (NULL shows the placeholder), freeing
	// whatever was shown before.
			void				SetBitmap(BBitmap* bitmap);
			void				Clear();

private:
			BBitmap*			fBitmap;
};

} // namespace tagkit

#endif // TAGKIT_COVER_ART_VIEW_H
