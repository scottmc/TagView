/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
// TagField.h
//
// Part of "tagkit" -- the reusable pieces of TagView.
// Names the tag fields a user can edit by hand, with helpers to read one
// out of a TagRecord as text and to put edited text back in (checking
// that numbers are numbers). Shared by the column list, the compact view
// and whatever edits the text, so they all agree on what "Track" means.

#ifndef TAGKIT_TAG_FIELD_H
#define TAGKIT_TAG_FIELD_H

#include <String.h>
#include <SupportDefs.h>

#include "TagRecord.h"


namespace tagkit {

enum tag_field {
	TAG_FIELD_ARTIST = 0,
	TAG_FIELD_TITLE,
	TAG_FIELD_ALBUM,
	TAG_FIELD_TRACK,
	TAG_FIELD_YEAR,
	TAG_FIELD_GENRE,

	TAG_FIELD_COUNT
};


// "Artist", "Title", ... -- the field's name as shown to the user.
const char* tag_field_label(tag_field field);

// The field's value as text. Track and Year come back blank when unknown
// (0), the same as they're shown everywhere else.
BString tag_field_value(const TagRecord& record, tag_field field);

// Stores text into the field (leading/trailing spaces dropped). Track and
// Year must be blank (= unknown) or a whole number from 0 to 9999;
// anything else returns false and leaves the record alone.
bool set_tag_field_value(TagRecord& record, tag_field field,
	const BString& text);

} // namespace tagkit

#endif // TAGKIT_TAG_FIELD_H
