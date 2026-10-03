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
#include <strings.h>

#include <Bitmap.h>
#include <BitmapStream.h>
#include <DataIO.h>
#include <Debug.h>
#include <File.h>
#include <NodeInfo.h>
#include <TranslationDefs.h>
#include <TranslationUtils.h>
#include <TranslatorFormats.h>
#include <TranslatorRoster.h>

#include <vector>


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


BString
image_extension_for_mime_type(const char* mimeType)
{
	BString mime(mimeType != NULL ? mimeType : "");
	mime.ToLower();
	if (!mime.StartsWith("image/"))
		return BString();

	static const struct {
		const char*	mime;
		const char*	extension;
	} kExtensions[] = {
		{ "image/jpeg", "jpg" },
		{ "image/jpg", "jpg" },
		{ "image/png", "png" },
		{ "image/gif", "gif" },
		{ "image/bmp", "bmp" },
		{ "image/x-bmp", "bmp" },
		{ "image/tiff", "tif" },
		{ "image/x-tiff", "tif" },
		{ "image/webp", "webp" },
		{ "image/x-webp", "webp" },
		{ "image/x-targa", "tga" },
		{ "image/x-tga", "tga" },
		{ "image/vnd.microsoft.icon", "ico" },
		{ "image/x-icon", "ico" },
		{ "image/x-photoshop", "psd" },
		{ "image/vnd.adobe.photoshop", "psd" },
		{ "image/jp2", "jp2" },
		{ "image/x-portable-pixmap", "ppm" },
		{ "image/x-ppm", "ppm" },
		{ "image/x-sgi", "sgi" },
		{ "image/x-sgi-bw", "sgi" },
		{ "image/x-pcx", "pcx" },
		{ "image/svg+xml", "svg" },
	};
	for (size_t i = 0; i < sizeof(kExtensions) / sizeof(kExtensions[0]); i++) {
		if (mime == kExtensions[i].mime)
			return BString(kExtensions[i].extension);
	}

	// Unknown to the table: use the subtype, without a leading "x-".
	BString subtype(mime.String() + 6);
	if (subtype.StartsWith("x-"))
		subtype.Remove(0, 2);
	int32 plus = subtype.FindFirst('+');
	if (plus > 0)
		subtype.Truncate(plus);
	return subtype;
}


status_t
load_cover_art_file(const char* path, CoverArtImage& image,
	BString* errorMessage)
{
	BFile file(path, B_READ_ONLY);
	status_t status = file.InitCheck();
	if (status != B_OK) {
		if (errorMessage != NULL)
			*errorMessage = "couldn't open the file";
		return status;
	}

	off_t size = 0;
	file.GetSize(&size);
	if (size <= 0 || size > 64 * 1024 * 1024) {
		if (errorMessage != NULL)
			*errorMessage = size <= 0 ? "the file is empty"
				: "the file is too big for cover art";
		return B_BAD_VALUE;
	}

	std::vector<unsigned char> data((size_t)size);
	if (file.Read(&data[0], (size_t)size) != (ssize_t)size) {
		if (errorMessage != NULL)
			*errorMessage = "couldn't read the file";
		return B_IO_ERROR;
	}

	return load_cover_art_data(data, image, errorMessage);
}


status_t
load_cover_art_data(std::vector<unsigned char>& data, CoverArtImage& image,
	BString* errorMessage)
{
	if (data.empty()) {
		if (errorMessage != NULL)
			*errorMessage = "there's no image data";
		return B_BAD_VALUE;
	}

	BString mime = detect_image_mime_type(&data[0], data.size());
	if (!mime.IsEmpty()) {
		image.mimeType = mime;
		image.data.swap(data);
		return B_OK;
	}

	// Some other format: decode with the translators and keep it as PNG.
	CoverArtImage original;
	original.data.swap(data);
	BBitmap* bitmap = decode_cover_art(original);
	if (bitmap == NULL) {
		if (errorMessage != NULL)
			*errorMessage = "that isn't an image format TagView can read";
		return B_NOT_SUPPORTED;
	}

	BTranslatorRoster* roster = BTranslatorRoster::Default();
	BMallocIO encoded;
	BBitmapStream stream(bitmap);	// owns the bitmap now
	status_t status = roster != NULL
		? roster->Translate(&stream, NULL, NULL, &encoded, B_PNG_FORMAT)
		: B_ERROR;
	BBitmap* leftover = NULL;
	stream.DetachBitmap(&leftover);
	delete leftover;

	if (status != B_OK || encoded.BufferLength() == 0) {
		if (errorMessage != NULL)
			*errorMessage = "couldn't convert that image to PNG";
		return status != B_OK ? status : B_ERROR;
	}

	const unsigned char* bytes
		= static_cast<const unsigned char*>(encoded.Buffer());
	image.data.assign(bytes, bytes + encoded.BufferLength());
	image.mimeType = "image/png";
	return B_OK;
}


BString
translator_output_mime_type(int32 translator, uint32 type)
{
	BTranslatorRoster* roster = BTranslatorRoster::Default();
	if (roster == NULL)
		return BString();

	const translation_format* formats = NULL;
	int32 count = 0;
	if (roster->GetOutputFormats(translator, &formats, &count) != B_OK
			|| formats == NULL) {
		return BString();
	}

	for (int32 i = 0; i < count; i++) {
		if (formats[i].type == type)
			return BString(formats[i].MIME);
	}
	return BString();
}


// Finds a translator that writes bitmaps as the given MIME type.
static bool
find_output_format(const char* mimeType, int32& translator, uint32& type)
{
	BTranslatorRoster* roster = BTranslatorRoster::Default();
	if (roster == NULL)
		return false;

	translator_id* ids = NULL;
	int32 idCount = 0;
	if (roster->GetAllTranslators(&ids, &idCount) != B_OK)
		return false;

	bool found = false;
	for (int32 i = 0; i < idCount && !found; i++) {
		const translation_format* formats = NULL;
		int32 count = 0;
		if (roster->GetOutputFormats(ids[i], &formats, &count) != B_OK)
			continue;

		for (int32 f = 0; f < count; f++) {
			if (formats[f].group == B_TRANSLATOR_BITMAP
					&& strcasecmp(formats[f].MIME, mimeType) == 0) {
				translator = ids[i];
				type = formats[f].type;
				found = true;
				break;
			}
		}
	}

	delete[] ids;
	return found;
}


status_t
encode_cover_art(const CoverArtImage& image, const char* mimeType,
	std::vector<unsigned char>& out)
{
	if (image.data.empty() || mimeType == NULL)
		return B_BAD_VALUE;

	if (strcasecmp(image.mimeType.String(), mimeType) == 0) {
		out = image.data;
		return B_OK;
	}

	int32 translator;
	uint32 type;
	if (!find_output_format(mimeType, translator, type))
		return B_NOT_SUPPORTED;

	BBitmap* bitmap = decode_cover_art(image);
	BTranslatorRoster* roster = BTranslatorRoster::Default();
	if (bitmap == NULL || roster == NULL) {
		delete bitmap;
		return B_ERROR;
	}

	BMallocIO encoded;
	BBitmapStream stream(bitmap);	// owns the bitmap now
	status_t status = roster->Translate(translator, &stream, NULL, &encoded,
		type);
	BBitmap* leftover = NULL;
	stream.DetachBitmap(&leftover);
	delete leftover;

	if (status != B_OK || encoded.BufferLength() == 0)
		return status != B_OK ? status : B_ERROR;

	const unsigned char* bytes
		= static_cast<const unsigned char*>(encoded.Buffer());
	out.assign(bytes, bytes + encoded.BufferLength());
	return B_OK;
}


status_t
export_cover_art(const CoverArtImage& image, const char* path,
	int32 translator, uint32 type, BString* errorMessage)
{
	BString mime = translator_output_mime_type(translator, type);

	BFile file(path, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	status_t status = file.InitCheck();
	if (status != B_OK) {
		if (errorMessage != NULL)
			*errorMessage = "couldn't create the file";
		return status;
	}

	if (!mime.IsEmpty() && mime == image.mimeType) {
		// Already in that format: no need to re-encode.
		if (file.Write(&image.data[0], image.data.size())
				!= (ssize_t)image.data.size()) {
			if (errorMessage != NULL)
				*errorMessage = "couldn't write the file";
			return B_IO_ERROR;
		}
	} else {
		BBitmap* bitmap = decode_cover_art(image);
		BTranslatorRoster* roster = BTranslatorRoster::Default();
		if (bitmap == NULL || roster == NULL) {
			delete bitmap;
			if (errorMessage != NULL)
				*errorMessage = "couldn't decode the cover art";
			return B_ERROR;
		}

		BBitmapStream stream(bitmap);	// owns the bitmap now
		status = roster->Translate(translator, &stream, NULL, &file, type);
		BBitmap* leftover = NULL;
		stream.DetachBitmap(&leftover);
		delete leftover;

		if (status != B_OK) {
			if (errorMessage != NULL)
				*errorMessage = "the translator couldn't write that format";
			return status;
		}
	}

	if (!mime.IsEmpty()) {
		BNodeInfo info(&file);
		info.SetType(mime.String());
	}
	return B_OK;
}


} // namespace tagkit
