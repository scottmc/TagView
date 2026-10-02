/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
#include "CoverArtFetch.h"
#include "ClientInfo.h"

#include <exception>
#include <stdio.h>

#include <Debug.h>

#include <coverart/CoverArt.h>



namespace tagkit {


int32
fetch_cover_art(const std::vector<ReleaseRef>& releases, int32 maxImages,
	const CoverArtFoundFunction& found)
{
	int32 count = 0;

	for (size_t i = 0; i < releases.size() && count < maxImages; i++) {
		const ReleaseRef& release = releases[i];
		if (release.id.IsEmpty())
			continue;

		std::vector<unsigned char> data;
		try {
			CoverArtArchive::CCoverArt coverArt(tagkit::user_agent());
			data = coverArt.FetchFront(release.id.String());
		} catch (std::exception& ex) {
			// Most often just "no cover art for this release" (a 404).
			fprintf(stderr, "TagView: no cover art for release %s (%s): %s\n",
				release.id.String(), release.title.String(), ex.what());
			continue;
		}

		if (data.empty()) {
			fprintf(stderr, "TagView: empty cover art for release %s (%s)\n",
				release.id.String(), release.title.String());
			continue;
		}

		CoverArtImage image;
		image.releaseId = release.id;
		image.releaseTitle = release.title;
		image.year = release.year;
		image.mimeType = detect_image_mime_type(&data[0], data.size());
		image.data.swap(data);

		// Anything that isn't a format we can both show and embed isn't
		// worth offering.
		if (image.mimeType.IsEmpty())
			continue;

		fprintf(stderr, "TagView: cover art for release %s (%s): %s, %d bytes\n",
			release.id.String(), release.title.String(),
			image.mimeType.String(), (int)image.data.size());
		count++;
		if (found)
			found(image);
	}

	return count;
}


std::vector<CoverArtImage>
fetch_cover_art(const std::vector<ReleaseRef>& releases, int32 maxImages)
{
	std::vector<CoverArtImage> images;
	fetch_cover_art(releases, maxImages,
		[&images](const CoverArtImage& image) { images.push_back(image); });
	return images;
}


} // namespace tagkit
