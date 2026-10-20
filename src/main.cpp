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
#include <string.h>

#include "tagkit/NetworkTimeout.h"
#include "tagkit/TagKitStrings.h"


// For testing tagkit::set_string_override(): with TAGVIEW_LABEL_TEST=1 the
// Artist, Title and Album words are replaced by "Interpret", "Titel" and
// "Platte" everywhere TagKit shows them.
static const char*
TestLabels(const char* english, const char* translated, void* cookie)
{
	if (strcmp(english, "Artist") == 0)
		return "Interpret";
	if (strcmp(english, "Title") == 0)
		return "Titel";
	if (strcmp(english, "Album") == 0)
		return "Platte";
	return NULL;
}


int
main()
{
	// For testing the network time limits: TAGVIEW_NETWORK_TIMEOUT=2
	// (seconds) makes every web lookup give up after 2 seconds.
	const char* timeout = getenv("TAGVIEW_NETWORK_TIMEOUT");
	if (timeout != NULL && atoi(timeout) > 0)
		tagkit::set_network_timeout(atoi(timeout));

	const char* labelTest = getenv("TAGVIEW_LABEL_TEST");
	if (labelTest != NULL && atoi(labelTest) > 0)
		tagkit::set_string_override(TestLabels);

	TagViewApp app;
	app.Run();
	return 0;
}
