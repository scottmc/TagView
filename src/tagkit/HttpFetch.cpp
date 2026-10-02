/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
#include "HttpFetch.h"

#include <stdio.h>
#include <string.h>
#include <sys/wait.h>


#define TAGVIEW_USER_AGENT "TagView-0.1 ( https://github.com/scottmc/TagView )"

// Downloads bigger than this are treated as an error (cover art is well
// under it).
static const size_t kMaxDownloadBytes = 32 * 1024 * 1024;


namespace tagkit {

namespace {

// The URL ends up inside a single-quoted shell argument, so only let
// through the characters a normal, already percent-encoded URL contains.
// (This also rules out quotes and anything else the shell could act on.)
bool
IsSafeUrl(const BString& url)
{
	if (url.FindFirst("http://") != 0 && url.FindFirst("https://") != 0)
		return false;

	for (int32 i = 0; i < url.Length(); i++) {
		unsigned char c = (unsigned char)url.ByteAt(i);
		bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
			|| (c >= '0' && c <= '9') || strchr("-._~:/?&=%+,;@!*()", c) != NULL;
		if (!ok)
			return false;
	}
	return true;
}

} // namespace


status_t
http_get(const BString& url, std::vector<unsigned char>& data,
	BString* errorMessage)
{
	data.clear();

	if (!IsSafeUrl(url)) {
		if (errorMessage != NULL)
			*errorMessage = "unsupported characters in URL";
		return B_BAD_VALUE;
	}

	// Haiku's own curl: follows redirects (-L), fails on HTTP errors (-f),
	// stays quiet but still reports errors (-sS, which go to stderr and
	// are dropped), and gives up after 20 seconds (-m).
	BString command("curl -sSfL -m 20 -A '");
	command << TAGVIEW_USER_AGENT << "' -- '" << url << "' 2>/dev/null";

	FILE* pipe = popen(command.String(), "r");
	if (pipe == NULL) {
		if (errorMessage != NULL)
			*errorMessage = "couldn't run curl";
		return B_ERROR;
	}

	unsigned char buffer[16384];
	size_t count;
	bool tooBig = false;
	while ((count = fread(buffer, 1, sizeof(buffer), pipe)) > 0) {
		if (data.size() + count > kMaxDownloadBytes) {
			tooBig = true;
			break;
		}
		data.insert(data.end(), buffer, buffer + count);
	}

	int status = pclose(pipe);

	if (tooBig) {
		data.clear();
		if (errorMessage != NULL)
			*errorMessage = "download too large";
		return B_ERROR;
	}

	if (status == -1 || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
		data.clear();
		if (errorMessage != NULL) {
			*errorMessage << "curl failed";
			if (status != -1 && WIFEXITED(status))
				*errorMessage << " (exit status " << (int32)WEXITSTATUS(status)
					<< ")";
		}
		return B_ERROR;
	}

	return B_OK;
}


} // namespace tagkit
