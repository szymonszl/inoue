# Inoue

Inoue is a program for automatically downloading [TETR.IO](https://tetr.io) Tetra League replays.

## Features

- it downloads replays
- easy to automate (crontab)
- runs on linux (might run on windows with msys or something, havent tested that yet)

## Guide

1. Create the directory you want to save your replays in.
1. Create a file called `inoue.cfg` in the directory. See the Configuration section for more information.
1. Compile the program. Builds are not available *yet*. Just clone/download the repo and run `make`. You will need libcurl on your system. You will get a binary as `build/inoue`.
1. Either copy the executable to the directory and call it as `cd /path/to/folder; ./inoue`, or call it as `/path/to/build/inoue /path/to/folder`. You can also install the executable globally and call it with the path, but doing so with `make install` is not supported yet.
1. Your replays should be downloaded! Please try viewing them to make sure that the download worked.

## Configuration

Inoue is controlled by a simple plaintext file called `inoue.cfg`. A simple example is provided as [inoue.cfg.example](inoue.cfg.example).

The file is structured in a simple `key value` format. You can add comments as lines starting in `#`, but they can't be placed in the same line as another directive (`username KAGARI # this is a comment` is not allowed).

Supported configuration fields:
- `username` - (required) the username of the user whose replays should be downloaded. Case insensitive.
- `token` - (required) your TETR.IO authentication token. **Case sensitive**. It *doesn't* have to be for the same account that is set in `username`. You can find it by viewing TETR.IO's Local Storage in your browser's developer tools (see instructions for [Chrome or Desktop](https://i.szymszl.xyz/1d72e5dd42.png))
- `filenameformat` - a strftime-style format string used for output filenames. All sequences in the format %a (percent symbol, letter) will be replaced with special values. Defaults to `%Y-%m-%d_%H-%M_%O.ttr`. See the table further.
- `useragent` - the browser [user agent](https://en.wikipedia.org/wiki/User_agent). Defaults to `Mozilla/5.0 (only pretending; Inoue/v1)`. You shouldn't have to change this.

In `filenameformat`, percent-letter sequences will be replaced as follows:

| sequence | replacement | example |
|----------|-------------|---------|
| `%u` | the user whose replay was downloaded, lowercase | `szy` |
| `%U` | the user whose replay was downloaded, uppercase | `SZY` |
| `%o` | the opponent of the user, lowercase | `osk` |
| `%O` | the opponent of the user, uppercase | `OSK` |
| `%r` | the full replay ID of the game | `5fe276147222310b7a4e1f33` |
| `%Y` | the year of the game | `2020` |
| `%y` | the two-digit year of the game | `20` |
| `%m` | the month of the game | `09` |
| `%d` | the day of the game | `14` |
| `%H` | the hour of the game | `19` |
| `%M` | the minute of the game | `37` |
| `%S` | the second of the game | `13` |
| `%s` | the [Unix timestamp](https://en.wikipedia.org/wiki/Unix_time) of the game | `1600112233` |
| `%%` | a single percent character | `%` |

The examples assume a game played by SZY against OSK on Mon Sep 14 07:37:13 PM 2020 (UTC). The date will be in the UTC timezone. **Invalid sequences will cause an error.**