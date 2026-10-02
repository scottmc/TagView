## Haiku Generic Build Makefile v2.6 ##

# Specify the name of the binary.
NAME = TagView

# Specify the type of binary, i.e. APP, SHARED (shared library), STATIC
# (static library) or DRIVER (kernel driver).
TYPE = APP

# If you plan to use localization, specify the application's MIME signature.
APP_MIME_SIG = application/x-vnd.scottmc-TagView

# Specify the source files to use. Full paths or paths relative to the
# Makefile can be used.
SRCS = \
	src/main.cpp \
	src/TagViewApp.cpp \
	src/TagViewWindow.cpp \
	src/SearchWindow.cpp \
	src/SearchResultsWindow.cpp \
	src/tagkit/TagRecord.cpp \
	src/tagkit/TagReader.cpp \
	src/tagkit/TagWriter.cpp \
	src/tagkit/CoverArtImage.cpp \
	src/tagkit/CoverArtFetch.cpp \
	src/tagkit/CoverArtCandidatesView.cpp \
	src/tagkit/CoverArtView.cpp \
	src/tagkit/HttpFetch.cpp \
	src/tagkit/ITunesArtwork.cpp \
	src/tagkit/CompactView.cpp \
	src/tagkit/TagField.cpp \
	src/CoverArtPickerWindow.cpp \
	src/FieldEditorWindow.cpp \
	src/tagkit/GenreList.cpp \
	src/tagkit/TagView.cpp \
	src/tagkit/MusicBrainzSearch.cpp \
	src/tagkit/RecordingMatchView.cpp \
	src/widgetkit/Barberpole.cpp \

# Specify the resource definition files to use. Full or relative paths can
# be used.
RDEFS =

# Specify the resource files to use. Full or relative paths can be used.
# Both RDEFS and RSRCS can be utilized in the same Makefile.
RSRCS =

# Specify additional libraries to link against. There are two acceptable
# forms of library specifications:
# - if your library follows the naming pattern of libXXX.so or libXXX.a
#   you can simply specify XXX for the library.
# - for any other library, you must specify the path to the library
#   and it's name, e.g. path/libname.a
LIBS = $(STDCPPLIBS) be tag translation tracker columnlistview musicbrainz5 \
	coverart

# Specify additional paths to directories following the standard libXXX.so
# or libXXX.a naming scheme. You can specify a full path or a path relative
# to the Makefile. The paths included may not be recursive, so include all
# paths where libraries need to be found. Directories where source files are
# found are automatically included.
LIBPATHS =

# Additional paths to look for system headers. These use the form
# #include <header>. Directories that contain the files in SRCS are
# NOT auto-included here.
# BColumnListView lives under Haiku's private interface headers (same as
# Hare's Makefile_Hare), not in the public headers directory.
SYSTEM_INCLUDE_PATHS = \
	$(shell findpaths -e B_FIND_PATH_HEADERS_DIRECTORY private/interface) \
	/boot/system/develop/headers/taglib

# Additional paths to look for local headers. These use the form
# #include "header". Directories that contain the files in SRCS are
# automatically included.
LOCAL_INCLUDE_PATHS = \
	src \
	src/tagkit \
	src/widgetkit \

# Specify the level of optimization that you want. Specify either NONE (do
# not optimize), ALL (optimize all the way), or a number from 0 to 3 to
# specify a level of optimization.
OPTIMIZE :=

# Specify the codes for languages you are going to support in this
# application. The default "en" one must be present, and it will be used if
# none other are found.
LOCALES =

# Specify all the libbe_localization_compiler linkables you'll need.
LOCALE_INCLUDE_PATHS =

# Specify any preprocessor symbols to be defined. The symbols will not have
# their values set automatically; you must supply the value (if any) to
# use. For example, use the define DEBUG=1 to provide the symbol DEBUG
# with a value of 1.
DEFINES =

# Specify special warning levels. Either ALL (enable all warnings), NONE
# (disable all warnings), or leave blank to use the default warnings.
WARNINGS =

# With image symbols, stack crawls in the debugger are meaningful. If
# you need to reduce the size of the executable, you can leave this
# blank, but it is preferable to specify "TRUE" here.
SYMBOLS :=

# Includes debug information, which allows the binary to be debugged
# easily. If set to "TRUE" the compiler flag "-g" is used.
DEBUGGER :=

# Specify any additional compiler flags to be used.
COMPILER_FLAGS =

# Specify any additional linker flags to be used.
LINKER_FLAGS =

# Specify the version of this binary. For more information: process_rev.
APP_VERSION :=

# (Only used when type is DRIVER. Specifies the desired driver install
# path relative to /dev.)
DRIVER_PATH =

## Include the Makefile-Engine
DEVEL_DIRECTORY := \
	$(shell findpaths -r "makefile_engine" B_FIND_PATH_DEVELOP_DIRECTORY)
include $(DEVEL_DIRECTORY)/etc/makefile-engine
