# Inoue

Inoue is a program for automatically downloading [TETR.IO](https://tetr.io) Tetra League replays.

## Features

- it downloads replays
- easy-ish to automate (crontab)
- runs on linux and windows!

## How does it work?

When you start Inoue, the program connects to TETR.IO and downloads your latest games that haven't been saved yet, and saves them as `.ttrm` files.
As long as you run Inoue at least once every 10 games, all your games with be backed up!

## Guide for Windows

1. Download and unpack the latest build from [Releases](https://github.com/szymonszl/inoue/releases).
2. The unpacked folder will contain a file called `inoue.cfg`. Please fill it in according to the **Configuration** section.
3. Run `inoue.bat`.
4. Your replays should be downloaded! Please try viewing them to make sure that the download worked.
5. Run `inoue.bat` every time you have played some more games and want to save them.

Note: If you are a developer and want to compile the program yourself, please look into [building_on_windows.md](building_on_windows.md).

## Guide for Linux

1. Create the directory you want to save your replays in.
2. Create a file called `inoue.cfg` in the directory. See the **Configuration** section for more information.
3. Compile the program. Builds are not available *yet*. Just clone/download the repo and run `make`. You will need libcurl on your system. You will get a binary as `build/inoue`.
4. Either copy the executable to the directory and call it as `cd /path/to/folder; ./inoue`, or call it as `/path/to/build/inoue /path/to/folder`. You can also install the executable globally and call it with the path, but doing so with `make install` is not supported yet.
5. Your replays should be downloaded! Please try viewing them to make sure that the download worked.
6. Run `inoue` every time you have played some more games and want to save them.

## Configuration

Inoue is controlled by a simple plaintext file called `inoue.cfg`. A simple example is provided as [inoue.cfg.example](inoue.cfg.example).

The file is structured in a simple `key value` format. You can add comments as lines starting in `#`, but they can't be placed in the same line as another directive (`username KAGARI # this is a comment` is not allowed).

Supported configuration fields:
- `username` - (required) the username of the user whose replays should be downloaded. Case insensitive.
- `token` - (required) your TETR.IO authentication token. **Case sensitive**. It *doesn't* have to be for the same account that is set in `username`. You can find it by viewing TETR.IO's Local Storage in your browser's developer tools (see instructions for [Chrome or Desktop](https://i.szymszl.xyz/1d72e5dd42.png))
- `filenameformat` - a strftime-style format string used for output filenames. All sequences in the format %a (percent symbol, letter) will be replaced with special values. Defaults to `%Y-%m-%d_%H-%M_%O.ttr`. See the table further.
- `useragent` - the browser [user agent](https://en.wikipedia.org/wiki/User_agent). Defaults to `Mozilla/5.0 (only pretending; Inoue/v1)`. You shouldn't have to change this.

In `filenameformat`, percent-letter sequences will be replaced as follows:

| sequence | replacement                                                               | example                    |
|----------|---------------------------------------------------------------------------|----------------------------|
| `%u`     | the user whose replay was downloaded, lowercase                           | `szy`                      |
| `%U`     | the user whose replay was downloaded, uppercase                           | `SZY`                      |
| `%o`     | the opponent of the user, lowercase                                       | `osk`                      |
| `%O`     | the opponent of the user, uppercase                                       | `OSK`                      |
| `%r`     | the full replay ID of the game                                            | `5fe276147222310b7a4e1f33` |
| `%Y`     | the year of the game                                                      | `2020`                     |
| `%y`     | the two-digit year of the game                                            | `20`                       |
| `%m`     | the month of the game                                                     | `09`                       |
| `%d`     | the day of the game                                                       | `14`                       |
| `%H`     | the hour of the game                                                      | `19`                       |
| `%M`     | the minute of the game                                                    | `37`                       |
| `%S`     | the second of the game                                                    | `13`                       |
| `%s`     | the [Unix timestamp](https://en.wikipedia.org/wiki/Unix_time) of the game | `1600112233`               |
| `%%`     | a single percent character                                                | `%`                        |

The examples assume a game played by SZY against OSK on Mon Sep 14 07:37:13 PM 2020 (UTC). The date will be in the UTC timezone. **Invalid sequences will cause an error.**

## Security

Inoue requires your user token in order to access the main game API. At the time, there is no other way to download replays. Your credentials aren't used for other reasons than to authenticate with the API.

If you don't trust Inoue with your main account, feel free to use another one! You don't have to use your account to download your own replays. Make sure that you're not violating the TETR.IO alt policy though!

## Acknowledgements

First of all, big thanks to osk for such an excellent game!

This program uses the excellent [curl](https://curl.se/) library to connect to the servers, and the great [json.h](https://github.com/sheredom/json.h) header-only library to parse JSON.
The Windows build uses NetBSD's [implementation of `strptime`](http://cvsweb.netbsd.org/bsdweb.cgi/src/lib/libc/time/strptime.c?rev=1.63&content-type=text/x-cvsweb-markup), with some slight modifications.
