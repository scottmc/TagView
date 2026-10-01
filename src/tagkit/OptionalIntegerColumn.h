/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
// OptionalIntegerColumn.h
//
// Part of "tagkit" -- the reusable pieces of TagView.
// A right-aligned numeric column for values where 0 (or less) means
// "unknown" -- track number, year -- so unknown shows as a blank cell
// rather than a misleading "0". Sorts numerically (blank first), unlike a
// plain BStringColumn which would put "10" before "9".
//
// Fill it with BStringFields built by format_optional_int().

#ifndef TAGKIT_OPTIONAL_INTEGER_COLUMN_H
#define TAGKIT_OPTIONAL_INTEGER_COLUMN_H

#include <stdlib.h>

#include <ColumnTypes.h>
#include <String.h>


namespace tagkit {


// "" for value <= 0 (unknown), otherwise the number as text.
inline BString
format_optional_int(int32 value)
{
	BString text;
	if (value > 0)
		text << value;
	return text;
}


class OptionalIntegerColumn : public BStringColumn {
public:
	OptionalIntegerColumn(const char* title, float width, float minWidth,
		float maxWidth)
		:
		BStringColumn(title, width, minWidth, maxWidth, B_TRUNCATE_END,
			B_ALIGN_RIGHT)
	{
	}

	virtual int CompareFields(BField* field1, BField* field2)
	{
		return _ValueOf(field1) - _ValueOf(field2);
	}

private:
	static int _ValueOf(BField* field)
	{
		const char* text = static_cast<BStringField*>(field)->String();
		return text != NULL ? atoi(text) : 0;
	}
};


} // namespace tagkit

#endif // TAGKIT_OPTIONAL_INTEGER_COLUMN_H
