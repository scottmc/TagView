/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
#include "CoverArtFetch.h"

#include <exception>

#include <Debug.h>

#include <coverart/CoverArt.h>

#define TAGVIEW_USER_AGENT "TagView-0.1 ( https://github.com/scottmc/TagView )"


namespace tagkit {


std::vector<CoverArtImage>
fetch_cover_art(const std::vector<ReleaseRef>& releases, int32 maxImages)
{
	std::vector<CoverArtImage> images;

	for (size_t i = 0; i < releases.size()
			&& (int32)images.size() < maxImages; i++) {
		const ReleaseRef& release = releases[i];
		if (release.id.IsEmpty())
			continue;

		std::vector<unsigned char> data;
		try {
			CoverArtArchive::CCoverArt coverArt(TAGVIEW_USER_AGENT);
			data = coverArt.FetchFront(release.id.String());
		} catch (std::exception& ex) {
			// Most often just "no cover art for this release" (a 404).
			PRINT(("CoverArtFetch: no cover for %s: %s\n",
				release.id.String(), ex.what()));
			continue;
		}

		if (data.empty())
			continue;

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

		images.push_back(image);
	}

	return images;
}


} // namespace tagkit
