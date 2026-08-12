<div align="center">

![Logo](resources/logo.png)

# qmfw

A C23 framework for games, software and everything in-between.

[![FOSSA Status](https://app.fossa.io/api/projects/git%2Bgithub.com%2FTalonBraveInfo%2Fplatform.svg?type=shield)](https://app.fossa.io/projects/git%2Bgithub.com%2FTalonBraveInfo%2Fplatform?ref=badge_shield)
[![CodeFactor](https://www.codefactor.io/repository/github/quartermindgames/hei/badge)](https://www.codefactor.io/repository/github/quartermindgames/hei)

[Features](#features) | [Roadmap](#roadmap) | [Users](#users) | [License](#license)

</div>

----

> [!CAUTION]
> This library is going through a significant refactor.
See [the roadmap](#roadmap) below for details.

A collection of relatively small utility libraries written in C23 that can be used as the foundation for various applications, though probably more oriented towards games.
Includes APIs for dealing with images, graphics, IO and much more.

Originally created around 2014/2015 as a support library for a game, it's since found a new life supporting [ApeTech](https://www.hogsy.me/ape.htm) and other projects.
Do let me know if you find any use of it, and I'll give you a shoutout!

**Keep in mind this is likely far from the most efficient library in the world.
If performance is a big priority for you, then I would highly recommend looking elsewhere!**

The qmfw library is mostly focused on supporting ApeTech at this time, and has such been cut down overtime to remove unnecessary features.
An older, less polished but larger version of this library, called Hei, is available [here](https://git.solemnwarning.net/hogsy/hei).
It supports a larger range of formats and is often used for my own reverse-engineering work, and will likely continue being developed in parallel.

# Features

The qmfw library is made up of the following.

- [qmfs](src/qmfs)
  - I/O focused library that provides a virtual file-system, allowing you to mount directories and packages.
  - Depends: qmos
- [qmgfx](src/qmgfx)
  - Graphics abstraction library. Currently, provides an interface that wraps OpenGL but is designed to be extended to support other APIs.
  - Depends: qmos
- [qmimg](src/qmimg)
  - Image-focused library allowing read/write support for a wide number of different formats.
  - Depends: qmos
- [qmmath](src/qmmath)
  - A small math library.
  - Depends: none!
- [qmos](src/qmos)
  - Takes care of some system-level abstraction, memory management, linked lists, string methods, time, shared pointers and more. Some of this may be split out in due time.
  - Depends: none!
- [qmparse](src/qmparse)
  - Provides a basic header-only parser.
  - Depends: none!
- [qmtest](src/qmtest)
  - Intended to be used internally for implementing tests for each library, but could technically be used for other things.
  - Depends: none!

----

Support levels for formats indicate the following.
- A: **Very good, perfect or near perfect support.**
- B: **Probably good enough, but not perfect.**
- C: **Experimental. You'll likely encounter issues.**
- F: **Non-functional / broken.**

### Supported image formats

| Format            | Usages                      | Support Level | Mode |
|-------------------|-----------------------------|---------------|------|
| PNG               |                             | A             | R/W  |
| BMP               |                             | A             | R/W  |
| GIF               |                             | A             | R    |
| TGA               |                             | A             | R/W  |
| PSD               |                             | A             | R    |
| JPG               |                             | A             | R/W  |
| HDR               |                             | A             | R    |
| DDS               |                             | C             | R    |
| QOI               |                             | A             | R/W  |
| 3dfx 3DF          |                             | C             | R    |
| Monolith DTX      | LithTech (game engine)      | C             | R    |
| Ritual FTX        |                             | A             | R    |
| Ritual SWL        |                             | B             | R    |
| Sony TIM          |                             | B             | R    |
| 3D Realms TEX     |                             | A             | R    |
| Angel Studios TEX | Angel Engine (game engine)  | C             | R    |
| Red Storm RSB     |                             | C             | R    |
| Valve VTX         | Source Engine (game engine) | C             | R    |
| Core HGT          | Herdy Gerdy                 | B             | R    |

### Supported package formats

| Format       | Usages                                                                | Support Level | Mode |
|--------------|-----------------------------------------------------------------------|---------------|------|
| ZIP          | General package container format                                      | B             | R    |
| MAD/MTD      | Hogs of War, Actua Soccer                                             | A             | R    |
| VPP          | Red Faction, Red Faction II, Summoner, The Punisher                   | A             | R    |
| WAD          | Doom, Quake, Half-Life                                                | A             | R    |
| PAK          | Quake and Half-Life                                                   | A             | R    |
| Fresh BIN    | FreshEngine                                                           | B             | R    |
| GRP          | Duke Nukem 3D                                                         | A             | R    |
| VPK          | Vampire The Masquerade Bloodlines / Troika                            | B             | R    |
| WFear INU    | White Fear (Cancelled)                                                | A             | R    |
| OPK          | Outcast                                                               | B             | R    |
| Amuze AHF    | Headhunter                                                            | B             | R    |
| Amuze AFS    | Headhunter                                                            | B             | R    |
| Angel DAT    | Angel Studios                                                         | B             | R    |
| CLU          | Herdy Gerdy                                                           | B             | R    |
| IBF          | Iron Storm                                                            | Extra         | R    |
| HAL          | Mortyr                                                                | A             | R    |
| RID/RIM      | Eradicator                                                            | A             | R    |
| FTactics PAK | Future Tactics                                                        | B             | R    |
| Kri WAD      | The Mark of Kri                                                       | B             | R    |
| AITD PAK     | Alone in the Dark                                                     | Extra         | R    |
| Ice3D DAT    | BioShock 3D                                                           | B             | R    |
| FF           | Outwars                                                               | Extra         | R    |
| Blitz DAT    | Titan A.E., Chicken Run                                               | Extra         | R    |
| SWAT WAD     | Okre Engine (SWAT Global Strike Team)                                 | C             | R    |
| Haven DAT    | Haven Call of the King                                                | C             | R    |
| VSR          | Sentient                                                              | C             | R    |
| ALL          | [The Last Job](https://www.gamesthatwerent.com/2024/09/the-last-job/) | B             | R    |
| P5CK         | Free Radical Design games                                             | B             | R    |

## plmodel

Model API supports static and animated per-vertex/skeletal formats.
This hasn't received as much attention so support is very limited.

| Format         | Usages                  | Support Level | Mode |
|----------------|-------------------------|---------------|------|
| 3D Realms' CPJ | Duke Nukem Forever 2001 | C             | R    |
| Cyclone MDL    |                         | C             | R    |
| Triton HDV     | Into the Shadows        | C             | R    |
| Unreal 3D      | Unreal Engine 1 + 2     | F             | R    |
| Valve SMD      | Source Engine           | C             | W    |

# Roadmap

As of July 2025, I've decided to start work on a radical overhaul of the library.

- [ ] Rename repository to qmfw / qmframework
- [x] Switch to C23
- [ ] Split plcore into multiple libraries (e.g., `qmimage`, `qmmath`, etc.)
- [ ] Refactor `plgraphics` into `qmgfx`, and slim it down
- [ ] Refactor `plmodel` into `qmmodel`
- [x] Discard `plwindow` api
- [ ] Ensure better test coverage

Also do note the following...

- *pl_console* API is being partially merged into [ApeTech](); variables and commands are being discarded entirely.
- Plugin API is being discarded.

# Users

- [hei-cmd](https://github.com/QuartermindGames/hei-cmd), a command-line utility for using features of Hei
- [ApeTech](https://www.hogsy.me/ape.htm), a 3D game engine written in C
- [OpenHoW](https://github.com/TalonBraveInfo/OpenHoW), a reimplementation of Hogs of War

# License

The project is licensed under [MIT](LICENSE).

Additional licences for libraries used by `plcore` can be found [here](docs/plcore).
