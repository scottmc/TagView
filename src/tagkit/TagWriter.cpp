/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
#include "TagWriter.h"

#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/tstring.h>


namespace tagkit {


static TagLib::String
to_taglib_string(const BString& string)
{
	return TagLib::String(string.String(), TagLib::String::UTF8);
}


bool
write_tags(const TagRecord& record, BString* errorMessage)
{
	if (record.path.IsEmpty()) {
		if (errorMessage != NULL)
			*errorMessage = "no file path";
		return false;
	}

	TagLib::FileRef file(record.path.String());
	if (file.isNull() || file.tag() == NULL) {
		if (errorMessage != NULL)
			*errorMessage = "couldn't open the file's tags";
		return false;
	}

	TagLib::Tag* tag = file.tag();
	tag->setArtist(to_taglib_string(record.artist));
	tag->setTitle(to_taglib_string(record.title));
	tag->setAlbum(to_taglib_string(record.album));
	tag->setGenre(to_taglib_string(record.genre));
	tag->setComment(to_taglib_string(record.comment));
	tag->setTrack(record.track > 0 ? (unsigned int)record.track : 0);
	tag->setYear(record.year > 0 ? (unsigned int)record.year : 0);

	if (!file.save()) {
		if (errorMessage != NULL) {
			*errorMessage = "couldn't write to the file "
				"(is it read-only?)";
		}
		return false;
	}

	return true;
}


} // namespace tagkit
