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

// A file extension (no dot) the common other systems recognise for an image
// MIME type: "image/jpeg" -> "jpg", "image/png" -> "png", "image/bmp" ->
// "bmp", "image/tiff" -> "tif", ... Falls back to the MIME subtype (without
// an "x-") and is empty for something that isn't an image type.
BString image_extension_for_mime_type(const char* mimeType);

// Reads an image file of any kind the Translation Kit understands into
// image. JPEG, PNG and GIF files are used byte for byte (so they go into
// the audio file unchanged); anything else (BMP, TIFF, WebP, ...) is
// converted to PNG. Returns B_OK, or an error with a short reason in
// errorMessage if one is given.
status_t load_cover_art_file(const char* path, CoverArtImage& image,
	BString* errorMessage = NULL);

// What a Translation Kit translator writes for type: its MIME type
// ("image/png"), or empty if unknown.
BString translator_output_mime_type(int32 translator, uint32 type);

// Writes image to path in the format given by translator/type (as chosen
// from BTranslationUtils::AddTranslationItems()). Sets the file's MIME
// type. When the format is the one the image is already in, its bytes are
// written untouched rather than re-encoded.
status_t export_cover_art(const CoverArtImage& image, const char* path,
	int32 translator, uint32 type, BString* errorMessage = NULL);

// Decodes the image into a bitmap with Haiku's Translation Kit. Returns
// NULL if it can't be decoded. The caller owns the bitmap.
BBitmap* decode_cover_art(const CoverArtImage& image);

} // namespace tagkit

#endif // TAGKIT_COVER_ART_IMAGE_H
