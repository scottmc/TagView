/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
#include "TagAttributes.h"

#include <stdlib.h>
#include <string.h>

#include <fs_attr.h>

#include <Node.h>
#include <TypeConstants.h>


namespace tagkit {

namespace {

enum source {
	SOURCE_ARTIST,
	SOURCE_ALBUM,
	SOURCE_TITLE,
	SOURCE_TRACK,
	SOURCE_YEAR,
	SOURCE_GENRE,
	SOURCE_COMMENT
};


struct AttributeSpec {
	const char*	name;
	type_code	type;		// B_STRING_TYPE or B_INT32_TYPE
	source		from;
};


// Haiku's names (as ArmyKnife uses them on Haiku).
const AttributeSpec kAttributes[] = {
	{ "Audio:Artist",	B_STRING_TYPE,	SOURCE_ARTIST },
	{ "Audio:Album",	B_STRING_TYPE,	SOURCE_ALBUM },
	{ "Media:Title",	B_STRING_TYPE,	SOURCE_TITLE },
	{ "Audio:Track",	B_INT32_TYPE,	SOURCE_TRACK },
	{ "Media:Year",		B_INT32_TYPE,	SOURCE_YEAR },
	{ "Media:Genre",	B_STRING_TYPE,	SOURCE_GENRE },
	{ "Media:Comment",	B_STRING_TYPE,	SOURCE_COMMENT }
};

const int32 kAttributeCount = sizeof(kAttributes) / sizeof(kAttributes[0]);


BString
text_for(const TagRecord& record, source from)
{
	switch (from) {
		case SOURCE_ARTIST:		return record.artist;
		case SOURCE_ALBUM:		return record.album;
		case SOURCE_TITLE:		return record.title;
		case SOURCE_GENRE:		return record.genre;
		case SOURCE_COMMENT:	return record.comment;
		default:				return BString();
	}
}


int32
number_for(const TagRecord& record, source from)
{
	return from == SOURCE_TRACK ? record.track : record.year;
}


// True if the attribute is already on the node with exactly this value
// (and type).
bool
attribute_matches(BNode& node, const AttributeSpec& spec,
	const BString& text, int32 number)
{
	attr_info info;
	if (node.GetAttrInfo(spec.name, &info) != B_OK || info.type != spec.type)
		return false;

	if (spec.type == B_INT32_TYPE) {
		int32 current = 0;
		if (info.size != (off_t)sizeof(current)
				|| node.ReadAttr(spec.name, spec.type, 0, &current,
					sizeof(current)) != (ssize_t)sizeof(current)) {
			return false;
		}
		return current == number;
	}

	// Strings are stored with their terminating NUL.
	if (info.size != (off_t)text.Length() + 1)
		return false;
	BString current;
	char* buffer = current.LockBuffer(info.size);
	if (buffer == NULL)
		return false;
	ssize_t got = node.ReadAttr(spec.name, spec.type, 0, buffer, info.size);
	current.UnlockBuffer(got > 0 ? (int32)got - 1 : 0);
	return got == (ssize_t)info.size && current == text;
}


// Whether the record has something to store for this attribute.
bool
has_value(const AttributeSpec& spec, const BString& text, int32 number)
{
	return spec.type == B_INT32_TYPE ? number > 0 : text.Length() > 0;
}

} // namespace


bool
tag_attributes_need_update(const TagRecord& record)
{
	if (record.path.Length() == 0)
		return false;

	BNode node(record.path.String());
	if (node.InitCheck() != B_OK)
		return false;

	for (int32 i = 0; i < kAttributeCount; i++) {
		const AttributeSpec& spec = kAttributes[i];
		BString text = text_for(record, spec.from);
		int32 number = number_for(record, spec.from);
		if (has_value(spec, text, number)
				&& !attribute_matches(node, spec, text, number)) {
			return true;
		}
	}
	return false;
}


status_t
copy_tags_to_attributes(const TagRecord& record, int32* written)
{
	if (written != NULL)
		*written = 0;

	BNode node(record.path.String());
	status_t status = node.InitCheck();
	if (status != B_OK)
		return status;

	for (int32 i = 0; i < kAttributeCount; i++) {
		const AttributeSpec& spec = kAttributes[i];
		BString text = text_for(record, spec.from);
		int32 number = number_for(record, spec.from);
		if (!has_value(spec, text, number)
				|| attribute_matches(node, spec, text, number)) {
			continue;
		}

		// Replace rather than overwrite, so a shorter value (or one that
		// was stored as another type) doesn't leave old data behind.
		node.RemoveAttr(spec.name);

		ssize_t result;
		ssize_t expected;
		if (spec.type == B_INT32_TYPE) {
			expected = sizeof(number);
			result = node.WriteAttr(spec.name, spec.type, 0, &number,
				expected);
		} else {
			expected = text.Length() + 1;
			result = node.WriteAttr(spec.name, spec.type, 0, text.String(),
				expected);
		}
		if (result != expected)
			return result < 0 ? (status_t)result : B_IO_ERROR;

		if (written != NULL)
			(*written)++;
	}
	return B_OK;
}

} // namespace tagkit
