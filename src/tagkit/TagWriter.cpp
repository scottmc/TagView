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
#include <taglib/tbytevector.h>
#include <taglib/tlist.h>
#include <taglib/tmap.h>
#include <taglib/tstring.h>
#include <taglib/tvariant.h>


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

	if (record.newCoverArt != NULL && !record.newCoverArt->data.empty()) {
		// Replace the file's front cover (if it has one) with the chosen
		// image, keeping any other embedded pictures (back cover, ...).
		const CoverArtImage& art = *record.newCoverArt;
		const TagLib::String kPictureKey("PICTURE");
		const TagLib::String kFrontCover("Front Cover");

		TagLib::List<TagLib::VariantMap> pictures;
		TagLib::List<TagLib::VariantMap> existing
			= file.complexProperties(kPictureKey);
		for (TagLib::List<TagLib::VariantMap>::ConstIterator it
				= existing.begin(); it != existing.end(); ++it) {
			if (it->value("pictureType").toString() != kFrontCover)
				pictures.append(*it);
		}

		TagLib::VariantMap picture;
		picture["data"] = TagLib::ByteVector(
			reinterpret_cast<const char*>(&art.data[0]),
			(unsigned int)art.data.size());
		picture["pictureType"] = kFrontCover;
		picture["mimeType"] = to_taglib_string(art.mimeType);
		picture["description"] = TagLib::String();
		pictures.append(picture);

		if (!file.setComplexProperties(kPictureKey, pictures)) {
			if (errorMessage != NULL)
				*errorMessage = "this file type can't hold cover art";
			return false;
		}
	}

	if (record.removeCoverArt && record.newCoverArt == NULL) {
		// Drop every embedded picture (the reader shows any of them when
		// there's no front cover, so a partial removal would just show
		// another one).
		if (!file.setComplexProperties(TagLib::String("PICTURE"),
				TagLib::List<TagLib::VariantMap>())) {
			if (errorMessage != NULL)
				*errorMessage = "couldn't remove the cover art";
			return false;
		}
	}

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
