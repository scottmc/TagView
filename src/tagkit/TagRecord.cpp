#include "TagRecord.h"

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
	hasCoverArt(false)
{
}


bool
TagRecord::IsUntagged() const
{
	return artist.IsEmpty() && title.IsEmpty();
}


} // namespace tagkit
