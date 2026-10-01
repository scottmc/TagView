/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
#include "TagRecord.h"

#include <ctype.h>
#include <string.h>


namespace tagkit {


static bool
ends_with_ci(const BString& str, const char* suffix)
{
	int32 suffixLen = (int32)strlen(suffix);
	int32 strLen = str.Length();
	if (strLen < suffixLen)
		return false;

	BString tail;
	str.CopyInto(tail, strLen - suffixLen, suffixLen);
	tail.ToLower();
	return tail == suffix;
}


audio_format
format_from_extension(const BString& path)
{
	if (ends_with_ci(path, ".mp3"))
		return AUDIO_FORMAT_MP3;
	if (ends_with_ci(path, ".ogg"))
		return AUDIO_FORMAT_OGG;
	if (ends_with_ci(path, ".flac"))
		return AUDIO_FORMAT_FLAC;
	return AUDIO_FORMAT_UNKNOWN;
}


const char*
format_label(audio_format format)
{
	switch (format) {
		case AUDIO_FORMAT_MP3:
			return "MP3";
		case AUDIO_FORMAT_OGG:
			return "Ogg Vorbis";
		case AUDIO_FORMAT_FLAC:
			return "FLAC";
		default:
			return "Unknown";
	}
}


TagRecord::TagRecord()
	:
	format(AUDIO_FORMAT_UNKNOWN),
	track(0),
	year(0),
	durationSeconds(-1),
	bitRateKbps(-1),
	sampleRateHz(-1),
	channels(-1),
	tagsLoaded(false),
	modified(false),
	hasCoverArt(false)
{
}


bool
TagRecord::IsUntagged() const
{
	return artist.IsEmpty() && title.IsEmpty();
}


bool
guess_artist_song_from_file_name(const BString& fileName, BString& artist,
	BString& song)
{
	BString name(fileName);

	// drop the extension
	int32 dot = name.FindLast('.');
	if (dot > 0)
		name.Truncate(dot);

	// drop a leading track number ("03 ", "03.", "03 - ", "03_", ...)
	int32 i = 0;
	while (i < name.Length() && isdigit((unsigned char)name[i]))
		i++;
	if (i > 0 && i < name.Length()) {
		int32 j = i;
		while (j < name.Length()
				&& (name[j] == ' ' || name[j] == '.' || name[j] == '-'
					|| name[j] == '_')) {
			j++;
		}
		if (j > i)
			name.Remove(0, j);
	}

	// try common artist/song separators, most specific first
	static const char* kSeparators[] = { " - ", " \xE2\x80\x93 " /* en dash */,
		" \xE2\x80\x94 " /* em dash */, "-", NULL };

	for (int32 s = 0; kSeparators[s] != NULL; s++) {
		int32 pos = name.FindFirst(kSeparators[s]);
		if (pos <= 0)
			continue;

		name.CopyInto(artist, 0, pos);
		song = name;
		song.Remove(0, pos + (int32)strlen(kSeparators[s]));

		artist.Trim();
		song.Trim();
		artist.ReplaceAll('_', ' ');
		song.ReplaceAll('_', ' ');

		if (artist.Length() > 0 && song.Length() > 0)
			return true;
	}

	return false;
}


} // namespace tagkit
