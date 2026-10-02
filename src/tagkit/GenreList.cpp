/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 *
 * The genre names come from Hare's GenreList.cpp:
 * Copyright 2000-2021, Hare Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include "GenreList.h"

#include <algorithm>
#include <string.h>
#include <strings.h>


namespace tagkit {

namespace {

// In ID3v1 order, as in Hare.
const char* const kGenres[] = {
	"Blues",  "Classic Rock",  "Country", "Dance",  "Disco", "Funk","Grunge",
	"Hip-Hop","Jazz", "Metal", "New Age", "Oldies","Other","Pop","R&B","Rap",
	"Reggae","Rock", "Techno","Industrial","Alternative","Ska","Death Metal",
	"Pranks",  "Soundtrack", "Euro-Techno",  "Ambient",  "Trip-Hop", "Vocal",
	"Jazz+Funk", "Fusion","Trance","Classical","Instrumental","Acid","House",
	"Games","Sound Clip", "Gospel","Noise","Alternative Rock","Bass", "Soul",
	"Punk", "Space",  "Meditative", "Instrumental Pop",  "Instrumental Rock",
	"Ethnic",   "Gothic",   "Darkwave",   "Techno-Industrial",  "Electronic",
	"Pop-Folk","Eurodance","Dream","Southern Rock","Comedy","Cult","Gangsta",
	"Top 40",   "Christian Rap",   "Jungle",  "Native American",   "Cabaret",
	"New Wave","Psychadelic","Rave","Showtunes", "Trailer", "Lo-Fi","Tribal",
	"Acid Punk",  "Acid Jazz",  "Polka",  "Retro", "Musical",  "Rock & Roll",
	"Hard Rock","Folk","Folk/Rock",	"National Folk", "Swing","Bebob","Latin",
	"Revival",    "Celtic",     "Bluegrass",   "Avantgarde",   "Gothic Rock",
	"Progressive Rock",  "Psychadelic Rock",  "Symphonic Rock",  "Slow Rock",
	"Big Band", "Chorus", "Easy Listening",  "Acoustic",  "Humour", "Speech",
	"Chanson", "Opera", "Chamber Music",  "Sonata",	"Symphony", "Booty Bass",
	"Primus", "Porn Groove", "Satire", "Slow Jam",  "Club", "Tango", "Samba",
	"Folklore"
};


bool
GenreComesBefore(const BString& a, const BString& b)
{
	return strcasecmp(a.String(), b.String()) < 0;
}

} // namespace


const std::vector<BString>&
genre_names()
{
	static std::vector<BString> names;
	if (names.empty()) {
		for (size_t i = 0; i < sizeof(kGenres) / sizeof(kGenres[0]); i++)
			names.push_back(BString(kGenres[i]));
		std::stable_sort(names.begin(), names.end(), GenreComesBefore);
	}
	return names;
}


} // namespace tagkit
