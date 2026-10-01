/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
#include "TagReader.h"

#include <taglib/audioproperties.h>
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/tstring.h>
#include <taglib/tstringlist.h>


namespace tagkit {


static BString
to_bstring(const TagLib::String& string)
{
	// to8Bit(true) == UTF-8, which is what BString/Haiku use throughout.
	BString result(string.to8Bit(true).c_str());
	result.Trim();
	return result;
}


bool
read_tags(TagRecord& record)
{
	if (record.path.IsEmpty())
		return false;

	TagLib::FileRef file(record.path.String());
	if (file.isNull())
		return false;

	TagRecord loaded(record);

	TagLib::Tag* tag = file.tag();
	if (tag != NULL) {
		loaded.artist = to_bstring(tag->artist());
		loaded.title = to_bstring(tag->title());
		loaded.album = to_bstring(tag->album());
		loaded.genre = to_bstring(tag->genre());
		loaded.comment = to_bstring(tag->comment());
		loaded.track = (int32)tag->track();
		loaded.year = (int32)tag->year();
	}

	TagLib::AudioProperties* properties = file.audioProperties();
	if (properties != NULL) {
		loaded.durationSeconds = properties->lengthInSeconds();
		loaded.bitRateKbps = properties->bitrate();
		loaded.sampleRateHz = properties->sampleRate();
		loaded.channels = properties->channels();
	}

	// "PICTURE" is TagLib's format-independent key for embedded cover art
	// (ID3v2 APIC frames, FLAC picture blocks, Ogg METADATA_BLOCK_PICTURE).
	loaded.hasCoverArt
		= file.complexPropertyKeys().contains(TagLib::String("PICTURE"));

	loaded.tagsLoaded = true;
	record = loaded;
	return true;
}


} // namespace tagkit
