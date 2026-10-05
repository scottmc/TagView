/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
// TagAttributes.h
//
// Part of "tagkit" -- the reusable pieces of TagView.
// Haiku keeps a copy of a music file's main tags as file attributes (the
// columns Tracker shows, and what queries search): Audio:Artist,
// Audio:Album, Media:Title, Audio:Track, Media:Year, Media:Genre and
// Media:Comment -- the same set ArmyKnife uses. These helpers compare a
// TagRecord with the attributes on its file and write the tags out as
// attributes.
//
// A blank tag (empty text, or a Track/Year of 0) never touches the file's
// attribute: an attribute is added or updated only when the tag has a
// value to put there.

#ifndef TAGKIT_TAG_ATTRIBUTES_H
#define TAGKIT_TAG_ATTRIBUTES_H

#include <String.h>
#include <SupportDefs.h>

#include "TagRecord.h"


namespace tagkit {


// True if writing the record's tags as attributes would add or change at
// least one attribute on its file (so false when the file already has
// them all, or the file can't be opened).
bool tag_attributes_need_update(const TagRecord& record);

// Adds the attributes the file doesn't have yet and updates the ones whose
// value differs from the record's tags; ones that already match are left
// alone. *written (if given) is set to how many were added or changed.
// Returns B_OK, or the error that stopped it.
status_t copy_tags_to_attributes(const TagRecord& record,
	int32* written = NULL);

} // namespace tagkit

#endif // TAGKIT_TAG_ATTRIBUTES_H
