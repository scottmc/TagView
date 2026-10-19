/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
#include "TagViewApp.h"

#include <stdlib.h>

#include "tagkit/NetworkTimeout.h"


int
main()
{
	// For testing the network time limits: TAGVIEW_NETWORK_TIMEOUT=2
	// (seconds) makes every web lookup give up after 2 seconds.
	const char* timeout = getenv("TAGVIEW_NETWORK_TIMEOUT");
	if (timeout != NULL && atoi(timeout) > 0)
		tagkit::set_network_timeout(atoi(timeout));

	TagViewApp app;
	app.Run();
	return 0;
}
