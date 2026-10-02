/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
// HttpFetch.h
//
// Part of "tagkit" -- the reusable pieces of TagView.
// One blocking HTTP(S) GET. Haiku's public network kit has no HTTP client
// (the BHttpRequest headers are private), so this runs the curl that ships
// with Haiku, which follows redirects and uses the system's trusted
// certificates. Kept as a single small function so the backend can be
// swapped without touching the callers.

#ifndef TAGKIT_HTTP_FETCH_H
#define TAGKIT_HTTP_FETCH_H

#include <vector>

#include <String.h>
#include <SupportDefs.h>


namespace tagkit {

// Downloads url into data (replacing its contents). Blocks until done, so
// call it from a worker thread, never the UI thread. Returns B_OK only for
// a successful (2xx) response; otherwise an error, with a short reason in
// errorMessage if one is given.
status_t http_get(const BString& url, std::vector<unsigned char>& data,
	BString* errorMessage = NULL);

} // namespace tagkit

#endif // TAGKIT_HTTP_FETCH_H
