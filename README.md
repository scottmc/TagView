# TagView

Music Tag Viewer for Haiku.

TagView is a Haiku test/demo app for viewing and editing digital music
tags (MP3, Ogg Vorbis, FLAC), built on top of open-source libraries
including TagLib, MusicBrainz (via libmusicbrainz5) and libcoverart.

If a dropped/opened file has no tags but its artist and song name can be
guessed from the file name, TagView can look the track up on MusicBrainz
and let you pick which result matches.

![MusicBrainz search results](docs/screenshot-search-results.png)

## Design

The app is deliberately split in two:

- **`src/tagkit/`** -- reusable pieces with no app-specific wiring:
  - `TagRecord` -- a plain data holder for the tag fields TagView knows
    about (artist, title, album, track, year, genre, duration, format,
    ...), independent of any particular tagging library.
  - `TagView` (the `tagkit::TagView` class) -- a `BColumnListView`
    pre-configured to display `TagRecord` rows.

  The idea is that other apps (Hare, ArmyKnife, ...) that just want a
  quick tag-listing widget can pull in `tagkit` directly.

- **`src/`** -- the app itself: `TagViewApp`, `TagViewWindow` (menu bar +
  a `tagkit::TagView`, File > Open... and drag-and-drop of refs) and
  `SearchWindow` (Edit > Search..., the Artist/Song/MusicBrainz dialog).

## Status

This is the first step: a working window with the menu bar, file open
(filtered to `*.mp3`, `*.ogg`, `*.flac`), drag-and-drop of files into the
list, and the Search... dialog's UI (Artist/Song fields, Search button
that enables once both are filled in). Actual tag reading (TagLib),
MusicBrainz lookups and cover art (libcoverart) are not wired up yet --
those are next.

## Building

Uses Haiku's generic build Makefile (Makefile-Engine):

```
make
```

The resulting binary is `./TagView`.
