/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
#include "CoverArtImage.h"

#include <string.h>

#include <Bitmap.h>
#include <BitmapStream.h>
#include <Debug.h>
#include <MemoryIO.h>
#include <TranslatorFormats.h>
#include <TranslatorRoster.h>


namespace tagkit {


BString
detect_image_mime_type(const unsigned char* data, size_t size)
{
	static const unsigned char kPng[] = { 0x89, 'P', 'N', 'G' };

	if (size >= 3 && data[0] == 0xFF && data[1] == 0xD8 && data[2] == 0xFF)
		return BString("image/jpeg");
	if (size >= sizeof(kPng) && memcmp(data, kPng, sizeof(kPng)) == 0)
		return BString("image/png");
	if (size >= 3 && memcmp(data, "GIF", 3) == 0)
		return BString("image/gif");

	return BString();
}


// Same approach as Hare's MusicBrainzLookup DecodeImage().
BBitmap*
decode_cover_art(const CoverArtImage& image)
{
	if (image.data.empty())
		return NULL;

	BMemoryIO memoryIO(&image.data[0], image.data.size());

	BTranslatorRoster* roster = BTranslatorRoster::Default();
	if (roster == NULL)
		return NULL;

	BBitmapStream stream;
	status_t status = roster->Translate(&memoryIO, NULL, NULL, &stream,
		B_TRANSLATOR_BITMAP);
	if (status != B_OK) {
		PRINT(("CoverArtImage: image decode failed: %s\n", strerror(status)));
		return NULL;
	}

	BBitmap* bitmap = NULL;
	stream.DetachBitmap(&bitmap);
	return bitmap;
}


} // namespace tagkit
