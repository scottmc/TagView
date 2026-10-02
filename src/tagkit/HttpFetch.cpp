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

#include <DataIO.h>
#include <HttpRequest.h>
#include <OS.h>
#include <Url.h>
#include <UrlProtocolRoster.h>
#include <UrlRequest.h>

#define TAGVIEW_USER_AGENT "TagView-0.1 ( https://github.com/scottmc/TagView )"


namespace tagkit {


status_t
http_get(const BString& url, std::vector<unsigned char>& data,
	BString* errorMessage)
{
	data.clear();

	BMallocIO output;
	BUrl target(url.String());

	BUrlRequest* request = BUrlProtocolRoster::MakeRequest(target, &output);
	if (request == NULL) {
		if (errorMessage != NULL)
			*errorMessage = "couldn't create a request";
		return B_ERROR;
	}

	BHttpRequest* httpRequest = dynamic_cast<BHttpRequest*>(request);
	if (httpRequest != NULL) {
		httpRequest->SetUserAgent(TAGVIEW_USER_AGENT);
		httpRequest->SetFollowLocation(true);
		httpRequest->SetTimeout(20 * 1000000LL);
	}

	status_t result = B_OK;
	thread_id thread = request->Run();
	if (thread < 0) {
		result = thread;
		if (errorMessage != NULL)
			*errorMessage = "couldn't start the request";
	} else {
		status_t threadStatus = B_OK;
		wait_for_thread(thread, &threadStatus);

		result = request->Status();
		if (result != B_OK) {
			if (errorMessage != NULL)
				*errorMessage << "request failed: " << strerror(result);
		} else if (httpRequest != NULL
				&& httpRequest->Result().StatusCode() != 200) {
			result = B_ERROR;
			if (errorMessage != NULL)
				*errorMessage << "HTTP status "
					<< (int32)httpRequest->Result().StatusCode();
		}
	}

	if (result == B_OK) {
		const unsigned char* bytes
			= static_cast<const unsigned char*>(output.Buffer());
		data.assign(bytes, bytes + output.BufferLength());
	}

	delete request;
	return result;
}


} // namespace tagkit
