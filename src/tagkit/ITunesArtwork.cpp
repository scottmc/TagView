/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
#include "ITunesArtwork.h"

#include <stdio.h>
#include <stdlib.h>

#include <vector>

#include "HttpFetch.h"


namespace tagkit {

namespace {

// Large enough to look good in the compact view and when embedded, small
// enough to download quickly.
const char* const kArtworkSize = "600x600bb";

const int32 kSearchLimit = 10;


struct Candidate {
	BString	collectionId;
	BString	title;
	BString	artistName;
	BString	artworkUrl;
	int32	year;
	bool	albumMatches;
};


BString
PercentEncode(const BString& text)
{
	static const char* hex = "0123456789ABCDEF";

	BString encoded;
	for (int32 i = 0; i < text.Length(); i++) {
		unsigned char c = (unsigned char)text.ByteAt(i);
		if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
				|| (c >= '0' && c <= '9') || c == '-' || c == '_'
				|| c == '.' || c == '~') {
			encoded << (char)c;
		} else {
			char buffer[4] = { '%', hex[c >> 4], hex[c & 15], '\0' };
			encoded << buffer;
		}
	}
	return encoded;
}


// Apple and MusicBrainz often write ’ where the tags have ', so compare
// with the curly quotes flattened (case is ignored by IFindFirst).
BString
NormalizeForMatch(const BString& text)
{
	BString normalized(text);
	normalized.ReplaceAll("\xE2\x80\x99", "'");
	normalized.ReplaceAll("\xE2\x80\x98", "'");
	return normalized;
}


void
AppendUtf8(BString& out, uint32 codePoint)
{
	char buffer[5];
	if (codePoint < 0x80) {
		buffer[0] = (char)codePoint;
		buffer[1] = '\0';
	} else if (codePoint < 0x800) {
		buffer[0] = (char)(0xC0 | (codePoint >> 6));
		buffer[1] = (char)(0x80 | (codePoint & 0x3F));
		buffer[2] = '\0';
	} else {
		buffer[0] = (char)(0xE0 | (codePoint >> 12));
		buffer[1] = (char)(0x80 | ((codePoint >> 6) & 0x3F));
		buffer[2] = (char)(0x80 | (codePoint & 0x3F));
		buffer[3] = '\0';
	}
	out << buffer;
}


// Finds "key":"value" inside one JSON object's text and returns the value
// with its escapes (\" \\ \/ \n \uXXXX) resolved. The objects iTunes
// returns for an album are flat, so a plain scan is enough -- no JSON
// library needed.
bool
FindJsonString(const BString& object, const char* key, BString& value)
{
	BString needle;
	needle << "\"" << key << "\"";
	int32 position = object.FindFirst(needle);
	if (position < 0)
		return false;

	position += needle.Length();
	while (position < object.Length() && (object.ByteAt(position) == ' '
			|| object.ByteAt(position) == ':')) {
		position++;
	}
	if (position >= object.Length() || object.ByteAt(position) != '"')
		return false;
	position++;

	value = "";
	while (position < object.Length()) {
		char c = object.ByteAt(position++);
		if (c == '"')
			return true;
		if (c != '\\') {
			value << c;
			continue;
		}

		if (position >= object.Length())
			break;
		char escaped = object.ByteAt(position++);
		switch (escaped) {
			case 'n':	value << '\n'; break;
			case 't':	value << '\t'; break;
			case 'u':
			{
				if (position + 4 > object.Length())
					return false;
				BString digits;
				object.CopyInto(digits, position, 4);
				AppendUtf8(value, (uint32)strtoul(digits.String(), NULL, 16));
				position += 4;
				break;
			}
			default:	value << escaped; break;	// \" \\ \/
		}
	}
	return false;
}


// The response is {"resultCount":N,"results":[{...},{...}]}; every album
// object starts with "wrapperType", so split the text there.
void
ParseCandidates(const BString& json, const BString& artist,
	const BString& album, std::vector<Candidate>& candidates)
{
	const char* marker = "{\"wrapperType\"";
	int32 start = json.FindFirst(marker);

	while (start >= 0) {
		int32 next = json.FindFirst(marker, start + 1);
		BString object;
		json.CopyInto(object, start, next >= 0 ? next - start
			: json.Length() - start);
		start = next;

		Candidate candidate;
		candidate.year = 0;
		candidate.albumMatches = false;

		BString releaseDate;
		if (!FindJsonString(object, "collectionName", candidate.title)
				|| !FindJsonString(object, "artistName",
					candidate.artistName)
				|| !FindJsonString(object, "artworkUrl100",
					candidate.artworkUrl)) {
			continue;
		}

		// Loose artist match, either way round (case-insensitive substring);
		// the search term itself already did the real filtering.
		BString wantedArtist = NormalizeForMatch(artist);
		BString candidateArtist = NormalizeForMatch(candidate.artistName);
		if (wantedArtist.Length() > 0
				&& candidateArtist.IFindFirst(wantedArtist) < 0
				&& wantedArtist.IFindFirst(candidateArtist) < 0) {
			continue;
		}

		if (FindJsonString(object, "releaseDate", releaseDate)
				&& releaseDate.Length() >= 4) {
			candidate.year = atoi(releaseDate.String());
		}

		// collectionId is a bare number, not a string.
		int32 idPosition = object.FindFirst("\"collectionId\"");
		if (idPosition >= 0) {
			idPosition += 14;
			while (idPosition < object.Length()
					&& (object.ByteAt(idPosition) == ' '
						|| object.ByteAt(idPosition) == ':')) {
				idPosition++;
			}
			while (idPosition < object.Length()
					&& object.ByteAt(idPosition) >= '0'
					&& object.ByteAt(idPosition) <= '9') {
				candidate.collectionId << object.ByteAt(idPosition++);
			}
		}

		candidate.albumMatches = album.Length() > 0
			&& NormalizeForMatch(candidate.title).IFindFirst(
				NormalizeForMatch(album)) >= 0;
		candidates.push_back(candidate);
	}
}


// ".../source/100x100bb.jpg" -> ".../source/600x600bb.jpg"
BString
LargeArtworkUrl(const BString& url)
{
	BString large(url);
	large.ReplaceFirst("100x100bb", kArtworkSize);
	return large;
}

} // namespace


int32
fetch_itunes_cover_art(const BString& artist, const BString& album,
	int32 maxImages, const CoverArtFoundFunction& found)
{
	if (artist.Length() == 0 || maxImages <= 0)
		return 0;

	BString term(artist);
	if (album.Length() > 0)
		term << " " << album;

	BString url("https://itunes.apple.com/search?media=music&entity=album"
		"&limit=");
	url << kSearchLimit << "&term=" << PercentEncode(term);

	std::vector<unsigned char> response;
	BString error;
	if (http_get(url, response, &error) != B_OK) {
		fprintf(stderr, "TagView: iTunes search failed: %s\n",
			error.String());
		return 0;
	}

	BString json;
	json.SetTo((const char*)&response[0], (int32)response.size());

	std::vector<Candidate> candidates;
	ParseCandidates(json, artist, album, candidates);
	fprintf(stderr, "TagView: iTunes found %d candidate albums for \"%s\"\n",
		(int)candidates.size(), term.String());

	// Albums whose title matches the one asked for go first.
	std::vector<Candidate> ordered;
	for (size_t i = 0; i < candidates.size(); i++) {
		if (candidates[i].albumMatches)
			ordered.push_back(candidates[i]);
	}
	for (size_t i = 0; i < candidates.size(); i++) {
		if (!candidates[i].albumMatches)
			ordered.push_back(candidates[i]);
	}

	int32 count = 0;
	for (size_t i = 0; i < ordered.size() && count < maxImages; i++) {
		const Candidate& candidate = ordered[i];

		std::vector<unsigned char> data;
		BString artworkError;
		if (http_get(LargeArtworkUrl(candidate.artworkUrl), data,
				&artworkError) != B_OK || data.empty()) {
			fprintf(stderr, "TagView: no iTunes artwork for \"%s\": %s\n",
				candidate.title.String(), artworkError.String());
			continue;
		}

		CoverArtImage image;
		image.releaseId << "itunes:" << candidate.collectionId;
		image.releaseTitle << candidate.title << " (iTunes)";
		image.year = candidate.year;
		image.mimeType = detect_image_mime_type(&data[0], data.size());
		image.data.swap(data);
		if (image.mimeType.IsEmpty())
			continue;

		fprintf(stderr, "TagView: iTunes cover for \"%s\": %s, %d bytes\n",
			candidate.title.String(), image.mimeType.String(),
			(int)image.data.size());
		count++;
		if (found)
			found(image);
	}

	return count;
}


} // namespace tagkit
