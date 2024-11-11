# Inoue

Inoue is a program for automatically downloading [TETR.IO](https://tetr.io) replays.

## How does it work?

When you run Inoue, the program connects to TETR.IO, downloads the replays you want, and saves them.
This includes both singleplayer (40L, Blitz) as well as multiplayer (Tetra League) games.
As long as you run Inoue at least once every 10 games, all your replays with be backed up!

## Features

- it downloads all kinds of replays
- runs on linux and windows
- fancy filenames
- what more could one need?

## Quick guide for Windows

1. Download and unpack the latest build from [Releases](https://github.com/szymonszl/inoue/releases).
2. The unpacked folder will contain a file called `inoue.txt`. Please fill it in according to the **Configuration** section.
3. Run `inoue.exe`.
4. Your replays should be downloaded! Please try viewing them to make sure that the download worked.
5. Run `inoue.exe` every time you have played some more games and want to save them.

To build, get MSYS2, run `make -f Makefile.win32 release`, and look for `build/inoue.zip`.

## Quick guide for Linux

1. Create the directory you want to save your replays in.
2. Create a file called `inoue.cfg` in the directory, and fill it in. See the **Configuration** section for more information.
3. Install Inoue's dependencies - a C compiler and curl's headers. On Ubuntu that can be installed as `sudo apt-get install build-essential libcurl4-openssl-dev`. On Archlikes try `sudo pacman -S --needed base-devel curl`.
4. Compile the program. Just `git clone` the repo and run `make`. You will get a binary as `build/inoue`.
5. Either copy the executable to the directory and call it as `cd /path/to/folder; ./inoue`, or call it as `/path/to/build/inoue /path/to/folder`, or install to PATH and use either of the above.
6. Your replays should be downloaded! Please try viewing them to make sure that the download worked.
7. Run `inoue` every time you have played some more games and want to save them.

## Configuration

A configuration file contains tasks (requests to download a set of replays), which are then executed by Inoue.
Every task needs to include the target user and the kind of replay to be downloaded, and optionally a description
how the file should be named.

Example configuration file:
```
user szy 40l saveas "%Y-%m-%d %H-%M 40L %T.ttr"
also blitz saveas "%Y-%m-%d %H-%M Blitz %b.ttr" # will assume szy from earlier task
also league # with default format
also user osk league saveas %u_vs_%o_%s.ttrm
```
This will download `szy`'s 40L games, named like `2020-10-22 10-48 40L 1'02.606.ttr`, then `szy`'s Blitz games named like `2020-05-12 10-25 Blitz 64886.ttr`, and finally `osk`'s Tetra League games under `osk_vs_paradoxiem_1653256831.ttrm`.

The user is marked by the word `user` followed by the username or userID, and the type of replay is marked by `40l`, `blitz`, `league`.
The description for the filename is marked by the word `saveas` followed by a format string, which can be quoted.
Tasks are delimited by the word `also`. If a task does not define a user, the user from the previous task is assumed.

Words and values are delimited by whitespace (spaces, newlines). Comments can be started with the `#` character and and last until the end of line,
but must be preceded by whitespace.

Filename formatting patterns (passed to `saveas`) can contain percent-letter sequences (as in, `%a`), which will be replaced by values as follows:

| sequence | replacement                                                               | availability | example                    |
|----------|---------------------------------------------------------------------------|--------------|----------------------------|
| `%u`     | the user whose replay was downloaded, lowercase                           | always       | `szy`                      |
| `%U`     | the user whose replay was downloaded, uppercase                           | always       | `SZY`                      |
| `%r`     | the full replay ID of the game                                            | always       | `5fe276147222310b7a4e1f33` |
| `%o`     | the opponent of the user, lowercase                                       | TL only      | `osk`                      |
| `%O`     | the opponent of the user, uppercase                                       | TL only      | `OSK`                      |
| `%b`     | the score gained in the replay                                            | SP only      | `64886`                    |
| `%t`     | the length of the game, in seconds with decimals                          | SP only      | `62.6067`                  |
| `%T`     | the length of the game, in minutes, seconds, and milliseconds             | SP only      | `1'02.606`                 |
| `%a`     | the altitude reached in Quick Play 2                                      | QP only      | `516`                      |
| `%Y`     | the year of the game                                                      | always       | `2020`                     |
| `%y`     | the two-digit year of the game                                            | always       | `20`                       |
| `%m`     | the month of the game                                                     | always       | `09`                       |
| `%d`     | the day of the game                                                       | always       | `14`                       |
| `%H`     | the hour of the game                                                      | always       | `19`                       |
| `%M`     | the minute of the game                                                    | always       | `37`                       |
| `%S`     | the second of the game                                                    | always       | `13`                       |
| `%s`     | the [Unix timestamp](https://en.wikipedia.org/wiki/Unix_time) of the game | not on Windows | `1600112233`             |
| `%%`     | a single percent character                                                | always       | `%`                        |

The multiplayer examples assume a game played by SZY against OSK on Mon Sep 14 07:37:13 PM 2020 (UTC). The date will be in the UTC timezone.
**Invalid sequences will cause an error**, so if you want to put a single percent sign in your filename please use `%%`.
Sequences not related to the type of game (like opponent name in singleplayer) will be replaced with nothing.

The filename pattern can also contain forward slashes `/` to mark directories. If the target directory does not exist, it will be automatically created.
For example, `%Y-%m/%Y-%m-%d %H-%M vs %T.ttrm` will categorize TL replays by their year and month, creating folders named like `2020-05`.

If a pattern is not specified with `saveas`, the following defaults are used:  
40L: `%Y-%m-%d_%H-%M_%T.ttr`  
Blitz: `%Y-%m-%d_%H-%M_%b.ttr`  
TL: `%Y-%m-%d_%H-%M_%O.ttrm`

## Command line options

```
Usage: inoue [-h] [-V] [-q] [[-c <command> | <path>]...]
```

Inoue can be provided with a path or paths to folders, which will be used as target directories, i.e. Inoue will look for files called
`inoue.cfg` or `inoue.txt` and use them. If no path is passed, the current working directory (the folder the program was ran from) is used instead,
as if `inoue .` was used.

Inoue can also be directly provided with instructions with the `-c` option. For example, `inoue -c 'user szy 40l'` will act as if
there was a configuration file with `user szy 40l` in the current working directory. The entire configuration string has to be passed as
one argument, so quoting is important. Multiple `-c` options can be passed.

Passing `-q` will enable quiet mode, which will make Inoue not print messages other than errors, as well as disable the `Press any key` prompt on Windows.
Useful for scripts or cron jobs.

Passing `-h` prints above synopsis and exits. Passing `-V` prints version information and exits.

## Replay API access

Currently replays are only available on TETR.IO through the main game API, which is illegal or at least very rude to use directly.
Earlier versions of Inoue did so, but that required passing it a token, and stopped working with an update which added new anticheat mitigations.
As such, I have set up an API at [inoue.szy.lol/api](https://inoue.szy.lol/api/), which uses a [bot account](https://ch.tetr.io/u/inoue_bot)
at the backend to download and forward replays. Inoue now uses this API.

If you're working on a project which requires TETR.IO replays, feel free to use the API! Visit the URL above for the documentation.

## Acknowledgements

First of all, big thanks to osk for such an excellent game!

This program uses the excellent [curl](https://curl.se/) library to connect to the servers on Linux,
and the great [json.h](https://github.com/sheredom/json.h) header-only library to parse JSON.
