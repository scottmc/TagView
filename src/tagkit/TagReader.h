/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
// TagReader.h
//
// Part of "tagkit" -- the reusable pieces of TagView.
// Reads an audio file's tags and audio properties with TagLib into a
// TagRecord. TagLib's own types stay private to TagReader.cpp, so callers
// (and other apps) only need TagRecord and don't have to include or
// understand TagLib's headers.

#ifndef TAGKIT_TAG_READER_H
#define TAGKIT_TAG_READER_H

#include "TagRecord.h"


namespace tagkit {

// Fills in record's tag fields (artist, title, album, genre, comment,
// track, year) and audio properties (duration, bit rate, sample rate,
// channels) from the file at record.path, then sets record.tagsLoaded.
// Fields the file has no value for are left at their TagRecord defaults.
// record.path, fileName and format are inputs and are left alone.
//
// Reads from disk, but only the file's headers/tags, so it's quick enough
// to call from the UI thread for the odd file; for a large batch, call it
// from a worker thread instead.
//
// Returns false (leaving record untouched) if the file couldn't be opened
// or TagLib doesn't recognize it as audio.
bool read_tags(TagRecord& record);

} // namespace tagkit

#endif // TAGKIT_TAG_READER_H
