/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
// ClientInfo.h
//
// Part of "tagkit" -- the reusable pieces of TagView.
// MusicBrainz, the Cover Art Archive and iTunes all ask API clients to say
// who they are (MusicBrainz will throttle or block anonymous ones), so tagkit
// identifies itself with the name and version of the program using it. The
// program provides them at build time, as defines in its Makefile:
//
//	DEFINES = TAGKIT_CLIENT_NAME=TagView TAGKIT_CLIENT_VERSION=0.1 \
//		TAGKIT_CLIENT_CONTACT=github.com/scottmc/TagView
//
// Write the values bare, without quotes. TAGKIT_CLIENT_NAME and
// TAGKIT_CLIENT_VERSION are required; TAGKIT_CLIENT_CONTACT (a web address
// or e-mail-like contact, without "https://") is optional but recommended,
// and MusicBrainz asks for it.
//
// If the name or version is missing the build still works, but the compiler
// prints a warning and, the first time the identity is used, tagkit prints
// a warning to stderr and falls back to "UnknownClient/0".
//
// tagkit::user_agent() gives the resulting User-Agent string, for example
// "TagView/0.1 ( github.com/scottmc/TagView )".

#ifndef TAGKIT_CLIENT_INFO_H
#define TAGKIT_CLIENT_INFO_H

#include <stdio.h>


#define TAGKIT_STRINGIFY_(x) #x
#define TAGKIT_STRINGIFY(x) TAGKIT_STRINGIFY_(x)

#if !defined(TAGKIT_CLIENT_NAME) || !defined(TAGKIT_CLIENT_VERSION)
#	warning "tagkit: define TAGKIT_CLIENT_NAME and TAGKIT_CLIENT_VERSION (e.g. DEFINES = TAGKIT_CLIENT_NAME=MyApp TAGKIT_CLIENT_VERSION=1.0) so MusicBrainz and iTunes can tell who is calling"
#	define TAGKIT_CLIENT_INFO_MISSING 1
#	ifndef TAGKIT_CLIENT_NAME
#		define TAGKIT_CLIENT_NAME UnknownClient
#	endif
#	ifndef TAGKIT_CLIENT_VERSION
#		define TAGKIT_CLIENT_VERSION 0
#	endif
#endif

#ifdef TAGKIT_CLIENT_CONTACT
#	define TAGKIT_CLIENT_CONTACT_PART \
		" ( " TAGKIT_STRINGIFY(TAGKIT_CLIENT_CONTACT) " )"
#else
#	define TAGKIT_CLIENT_CONTACT_PART ""
#endif

#define TAGKIT_USER_AGENT \
	TAGKIT_STRINGIFY(TAGKIT_CLIENT_NAME) "/" \
	TAGKIT_STRINGIFY(TAGKIT_CLIENT_VERSION) TAGKIT_CLIENT_CONTACT_PART


namespace tagkit {

// The User-Agent to send with every web request. Warns once on stderr if
// the program didn't provide its name and version.
inline const char*
user_agent()
{
#ifdef TAGKIT_CLIENT_INFO_MISSING
	static bool warned = false;
	if (!warned) {
		warned = true;
		fprintf(stderr, "tagkit: warning: TAGKIT_CLIENT_NAME and "
			"TAGKIT_CLIENT_VERSION were not defined at build time; "
			"identifying as \"%s\"\n", TAGKIT_USER_AGENT);
	}
#endif
	return TAGKIT_USER_AGENT;
}

} // namespace tagkit

#endif // TAGKIT_CLIENT_INFO_H
