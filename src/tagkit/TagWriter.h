/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
// TagWriter.h
//
// Part of "tagkit" -- the reusable pieces of TagView.
// The counterpart to TagReader: writes a TagRecord's tag fields back into
// the audio file with TagLib. TagLib's types stay private to
// TagWriter.cpp.

#ifndef TAGKIT_TAG_WRITER_H
#define TAGKIT_TAG_WRITER_H

#include "TagRecord.h"


namespace tagkit {

// Writes record's artist, title, album, genre, comment, track and year into
// the file at record.path. Empty strings and a track/year of 0 clear that
// field in the file. Audio data is never touched, and the record's other
// fields (duration, bit rate, ...) are read-only properties of the file so
// they are not written.
//
// This modifies the file on disk. It does blocking file I/O (rewriting the
// tag, and for some formats padding or the whole file), so for more than a
// file or two call it from a worker thread.
//
// Returns true on success. On failure returns false and, if errorMessage is
// not NULL, sets it to a short reason suitable for showing to the user
// (couldn't open the file, or couldn't save it -- most often because it's
// read-only or on a read-only volume).
bool write_tags(const TagRecord& record, BString* errorMessage = NULL);

} // namespace tagkit

#endif // TAGKIT_TAG_WRITER_H
