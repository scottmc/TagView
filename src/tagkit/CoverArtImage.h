/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
// CoverArtImage.h
//
// Part of "tagkit" -- the reusable pieces of TagView.
// One cover art image: the compressed bytes exactly as the Cover Art
// Archive served them (JPEG/PNG/GIF), plus which MusicBrainz release it
// belongs to. Kept as raw bytes so the very same image can be shown (decode
// it to a BBitmap) and embedded in a file's tags unchanged, with no lossy
// re-encode.

#ifndef TAGKIT_COVER_ART_IMAGE_H
#define TAGKIT_COVER_ART_IMAGE_H

#include <vector>

#include <String.h>
#include <SupportDefs.h>

class BBitmap;


namespace tagkit {

struct CoverArtImage {
	CoverArtImage()
		:
		year(0)
	{
	}

	BString		releaseId;		// MusicBrainz release this cover is from
	BString		releaseTitle;
	int32		year;			// 0 == unknown
	BString		mimeType;		// "image/jpeg", "image/png", "image/gif"
	std::vector<unsigned char>	data;	// the compressed image bytes
};


// "image/jpeg", "image/png" or "image/gif" judging by the data's leading
// bytes, or "" if it's none of those.
BString detect_image_mime_type(const unsigned char* data, size_t size);

// Decodes the image into a bitmap with Haiku's Translation Kit. Returns
// NULL if it can't be decoded. The caller owns the bitmap.
BBitmap* decode_cover_art(const CoverArtImage& image);

} // namespace tagkit

#endif // TAGKIT_COVER_ART_IMAGE_H
