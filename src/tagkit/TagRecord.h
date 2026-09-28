// TagRecord.h
//
// Part of "tagkit" -- the reusable pieces of TagView.
// TagRecord is a plain data holder for the tag fields we know how to show
// and edit. It intentionally has no dependency on any particular tagging
// library (TagLib, MusicBrainz, etc.) so it can be passed around freely
// and reused by other apps (Hare, ArmyKnife, ...).

#ifndef TAGKIT_TAG_RECORD_H
#define TAGKIT_TAG_RECORD_H

#include <String.h>
#include <SupportDefs.h>


namespace tagkit {

enum audio_format {
	AUDIO_FORMAT_UNKNOWN = 0,
	AUDIO_FORMAT_MP3,
	AUDIO_FORMAT_OGG,
	AUDIO_FORMAT_FLAC
};


// Returns the audio_format implied by a file's extension (".mp3", ".ogg",
// ".flac", case-insensitive). Returns AUDIO_FORMAT_UNKNOWN for anything
// else.
audio_format format_from_extension(const BString& path);

// Human readable label for an audio_format ("MP3", "Ogg Vorbis", "FLAC",
// "Unknown").
const char* format_label(audio_format format);


struct TagRecord {
	TagRecord();

	BString		path;			// full path of the source file
	BString		fileName;		// leaf name, for display
	audio_format	format;

	BString		artist;
	BString		title;
	BString		album;
	BString		genre;
	BString		comment;

	int32		track;			// 0 == unknown
	int32		year;			// 0 == unknown
	int32		durationSeconds;	// -1 == unknown
	int32		bitRateKbps;		// -1 == unknown
	int32		sampleRateHz;		// -1 == unknown
	int32		channels;		// -1 == unknown

	bool		tagsLoaded;		// true once a reader has filled this in
	bool		hasCoverArt;

	// True when neither artist nor title could be read from the file's
	// tags. This is the trigger for offering a MusicBrainz lookup based
	// on the file name instead.
	bool IsUntagged() const;
};

} // namespace tagkit

#endif // TAGKIT_TAG_RECORD_H
