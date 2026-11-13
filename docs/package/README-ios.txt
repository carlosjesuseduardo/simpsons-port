The Simpsons Arcade and The Simpsons for iPhone and iPad
=======================================================

Konami's The Simpsons (1991), the arcade game for four players, as a native game: decompiled to C and
checked frame by frame against MAME, github.com/cheviman/simpsons-port. The Simpsons Arcade is the arcade
game as it is; The Simpsons, the console version (menus of its own, no coins: START brings a player in).
This is the package for iOS and iPadOS (13 or later, an iPhone or an iPad of the last years: arm64): the
two games as .ipa files, without their assets and not signed. Nothing of the game comes in this package:
the games take their graphics and sounds out of your own ROM set.

They are not on the App Store, and never will be: the game is Konami's, and you bring the ROM set. You
install them yourself ("sideloading"), and the tool you install them with signs them for your device.

What you need
-------------
- simpsons.zip, MAME's ROM set of The Simpsons (4 Players World, set 1), as MAME has it;
- an iPhone or an iPad, and a computer (Windows or macOS) for the tool that installs the games.

Installing
----------
The .ipa files are in ios/: The Simpsons.ipa and The Simpsons Arcade.ipa. Install either or both with one
of these, which sign an app with your own Apple ID as they install it:

- AltStore (altstore.io) or SideStore (sidestore.io): install it on the device as its site says, then open
  the .ipa with it (AltStore's My Apps, the + at the top).
- Sideloadly (sideloadly.io), on a computer: the device on USB, drop the .ipa on its window, your Apple ID,
  Start.

With a free Apple ID an app installed so stops opening after 7 days: open AltStore or SideStore to refresh
it (they do it themselves in the background with their companion on the computer), or install it again
with Sideloadly; a refresh keeps the game's folder as it was. A paid developer account signs for a year. Where the iOS version allows it, TrollStore installs an .ipa for good.

The first time on iOS 16 or later, the device asks for Developer Mode (Settings, Privacy & Security,
Developer Mode) and to trust your Apple ID (Settings, General, VPN & Device Management).

The ROM set
-----------
Each game has its own folder, which the Files app shows: open the game once (it says it has no ROM set),
then in the Files app go to On My iPhone (On My iPad), The Simpsons (or The Simpsons Arcade), and put
simpsons.zip there. From a computer: Finder (or the Apple Devices app on Windows), the device, Files, the
game, and drop simpsons.zip on it. Open the game again: the first start takes the assets out of
simpsons.zip into one file, simpsons.assets, in that same folder (some seconds); from then on the game
starts from it, and simpsons.zip can go. Two games, two folders: each needs its own copy.

What the games make from your ROM set is yours alone: never share it.

Playing
-------
The touch screen has a stick where your left thumb lands, ATTACK and JUMP at the right, and START (COIN on
The Simpsons Arcade, where ATTACK then brings Marge in). A game controller (MFi, Xbox, PlayStation, Switch
Pro) hides them, and each controller is a player (player 1 is Marge, 2 Homer, 3 Bart, 4 Lisa): the stick or
the d-pad moves, A attacks, B jumps, Menu is START (and pauses The Simpsons), and on The Simpsons Arcade the
Options or View button puts a coin in. Going to the home screen pauses The Simpsons; swipe up from the
bottom (twice: the first swipe only shows the bar) to leave either game. The games keep their settings and
their logs (simpsons.log, simpsons-arcade.log) in their folders, beside simpsons.assets.

Licenses
--------
LICENSE.txt is the project's (BSD-3-Clause). licenses/ holds MAME's and ymfm's (the board's video and
sound follow their behavior and tables), SDL2's and SDL_GameControllerDB's (zlib: inside the games) and
Kenney's (CC0: the letters, the controls' pictures).

The Simpsons is a trademark of its owner, and the arcade game is Konami's; iPhone, iPad and iOS are
trademarks of Apple. This project is not affiliated with or endorsed by them.
