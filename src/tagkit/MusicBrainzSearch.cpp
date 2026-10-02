/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
#include "MusicBrainzSearch.h"
#include "ClientInfo.h"

#include <algorithm>
#include <exception>
#include <stdlib.h>

#include <Debug.h>

#include <musicbrainz5/ArtistCredit.h>
#include <musicbrainz5/Medium.h>
#include <musicbrainz5/MediumList.h>
#include <musicbrainz5/Metadata.h>
#include <musicbrainz5/NameCredit.h>
#include <musicbrainz5/NameCreditList.h>
#include <musicbrainz5/Query.h>
#include <musicbrainz5/Recording.h>
#include <musicbrainz5/RecordingList.h>
#include <musicbrainz5/Release.h>
#include <musicbrainz5/ReleaseList.h>
#include <musicbrainz5/Track.h>
#include <musicbrainz5/TrackList.h>



namespace tagkit {

namespace {

// How many of a recording's releases to keep for cover art lookup. Popular
// recordings can appear on dozens of compilations; the first few (MusicBrainz
// lists the most relevant first) are plenty to choose between.
const int32 kMaxReleasesPerMatch = 15;


BString
EscapeLuceneQuoted(const BString& text)
{
	BString result(text);
	result.ReplaceAll("\\", "\\\\");
	result.ReplaceAll("\"", "\\\"");
	return result;
}


// Orders matches by how far their length is from a target, closest first.
// Recordings with no known length sort after every one that has one.
struct CloserToTarget {
	CloserToTarget(int32 target) : fTarget(target) {}

	bool operator()(const RecordingMatch& a, const RecordingMatch& b) const
	{
		if (a.durationSeconds < 0 || b.durationSeconds < 0)
			return a.durationSeconds >= 0 && b.durationSeconds < 0;
		return abs(a.durationSeconds - fTarget)
			< abs(b.durationSeconds - fTarget);
	}

private:
	int32	fTarget;
};


// Joins a CArtistCredit's name-credit list into the single display
// string MusicBrainz itself uses for an artist credit (handles
// multi-artist collaborations, e.g. "Artist One feat. Artist Two") --
// same approach as Hare's MusicBrainzLookup::JoinArtistCredit().
BString
JoinArtistCredit(MusicBrainz5::CArtistCredit* credit)
{
	BString result;

	if (credit == NULL || credit->NameCreditList() == NULL)
		return result;

	MusicBrainz5::CNameCreditList* names = credit->NameCreditList();
	for (int i = 0; i < names->NumItems(); i++) {
		MusicBrainz5::CNameCredit* nameCredit = names->Item(i);
		if (nameCredit == NULL)
			continue;
		result << nameCredit->Name().c_str();
		result << nameCredit->JoinPhrase().c_str();
	}

	return result;
}


} // namespace


std::vector<RecordingMatch>
MusicBrainzSearch::SearchRecording(const BString& artist, const BString& song,
	const BString& album, int32 maxResults, int32 targetSeconds)
{
	std::vector<RecordingMatch> matches;

	if (artist.Length() == 0 || song.Length() == 0)
		return matches;

	MusicBrainz5::CQuery query(tagkit::user_agent());

	try {
		MusicBrainz5::CQuery::tParamMap searchParams;

		BString luceneQuery;
		luceneQuery << "artist:\"" << EscapeLuceneQuoted(artist)
			<< "\" AND recording:\"" << EscapeLuceneQuoted(song) << "\"";
		if (album.Length() > 0)
			luceneQuery << " AND release:\"" << EscapeLuceneQuoted(album)
				<< "\"";
		searchParams["query"] = luceneQuery.String();

		PRINT(("MusicBrainzSearch: %s\n", luceneQuery.String()));

		// Kept alive for the rest of the function -- candidateReleases-
		// style pattern from Hare's MusicBrainzLookup: the list pointer
		// below points into this, so it has to outlive the lookup.
		MusicBrainz5::CMetadata result;
		result = query.Query("recording", "", "", searchParams);

		MusicBrainz5::CRecordingList* recordings = result.RecordingList();
		if (recordings == NULL) {
			PRINT(("MusicBrainzSearch: no recordings found\n"));
			return matches;
		}

		// When sorting by length, look at everything MusicBrainz sent back
		// (not just the first maxResults) so a close-length match further
		// down its relevance list can still make the cut.
		int32 count = recordings->NumItems();
		if (targetSeconds < 0 && count > maxResults)
			count = maxResults;

		for (int32 i = 0; i < count; i++) {
			MusicBrainz5::CRecording* recording = recordings->Item(i);
			if (recording == NULL)
				continue;

			RecordingMatch match;
			match.id = recording->ID().c_str();
			match.title = recording->Title().c_str();
			match.artist = JoinArtistCredit(recording->ArtistCredit());
			match.durationSeconds = recording->Length() > 0
				? recording->Length() / 1000 : -1;

			// The relevance score MusicBrainz's search returns
			// ("ext:score" in the raw XML) isn't exposed through this
			// libmusicbrainz5 build's public API -- its parser logs it
			// as an "Unrecognised recording attribute" and doesn't
			// carry it into CEntity::ExtAttributes() either. Left at
			// -1 (RecordingMatch's default); the results are still
			// returned in MusicBrainz's own relevance order, so list
			// position itself conveys the ranking.
			match.score = -1;

			MusicBrainz5::CReleaseList* releases = recording->ReleaseList();
			if (releases != NULL && releases->NumItems() > 0
					&& releases->Item(0) != NULL) {
				// With an album given, the release whose title contains it
				// is the one to report (and to try for cover art first);
				// otherwise it's MusicBrainz's first release.
				int32 chosen = 0;
				if (album.Length() > 0) {
					for (int32 r = 0; r < releases->NumItems(); r++) {
						MusicBrainz5::CRelease* each = releases->Item(r);
						if (each == NULL)
							continue;
						BString eachTitle = each->Title().c_str();
						if (eachTitle.IFindFirst(album) >= 0) {
							chosen = r;
							break;
						}
					}
				}

				// Remember every release (capped) for cover art lookup,
				// the chosen one first.
				for (int32 n = 0; n < releases->NumItems()
						&& (int32)match.releases.size() < kMaxReleasesPerMatch;
						n++) {
					int32 r = n == 0 ? chosen : (n <= chosen ? n - 1 : n);
					MusicBrainz5::CRelease* each = releases->Item(r);
					if (each == NULL)
						continue;

					ReleaseRef ref;
					ref.id = each->ID().c_str();
					ref.title = each->Title().c_str();
					BString eachDate = each->Date().c_str();
					if (eachDate.Length() >= 4)
						ref.year = atoi(eachDate.String());
					match.releases.push_back(ref);
				}

				MusicBrainz5::CRelease* release = releases->Item(chosen);
				if (release == NULL)
					release = releases->Item(0);
				match.album = release->Title().c_str();

				// Release date is "YYYY", "YYYY-MM" or "YYYY-MM-DD" --
				// or empty when MusicBrainz doesn't know it.
				BString date = release->Date().c_str();
				if (date.Length() >= 4)
					match.year = atoi(date.String());

				// Where this recording sits on that release: the first
				// track of the first disc that lists it. Left at 0 if
				// the response didn't carry a track list.
				MusicBrainz5::CMediumList* media = release->MediumList();
				for (int32 m = 0; media != NULL && match.track == 0
						&& m < media->NumItems(); m++) {
					MusicBrainz5::CMedium* medium = media->Item(m);
					if (medium == NULL || medium->TrackList() == NULL
							|| medium->TrackList()->NumItems() == 0
							|| medium->TrackList()->Item(0) == NULL) {
						continue;
					}
					match.track = medium->TrackList()->Item(0)->Position();
				}
			}

			matches.push_back(match);
		}

		if (targetSeconds >= 0) {
			// stable_sort so equally-close matches keep MusicBrainz's
			// relevance order.
			std::stable_sort(matches.begin(), matches.end(),
				CloserToTarget(targetSeconds));
		}
		if ((int32)matches.size() > maxResults)
			matches.resize(maxResults);
	} catch (std::exception& ex) {
		PRINT(("MusicBrainzSearch: search failed: %s\n", ex.what()));
		matches.clear();
	}

	return matches;
}

} // namespace tagkit
