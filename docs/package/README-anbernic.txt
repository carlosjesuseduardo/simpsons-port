The Simpsons Arcade and The Simpsons for Anbernic's H700 handhelds
==================================================================

Konami's The Simpsons (1991), the arcade game for four players, as a native game: decompiled to C and
checked frame by frame against MAME, github.com/cheviman/simpsons-port. The Simpsons Arcade is the arcade
game as it is; The Simpsons, the console version (menus of its own, no coins: START brings a player in).
This is the package for the stock firmware of Anbernic's H700 handhelds (RG35XX Plus, RG40XX H and V, RG
CubeXX and their kin): the games without their assets, ready for the card, and the tool that makes your
PORTS of them on your computer. Nothing of the game comes in this package: the games take their graphics
and sounds out of your own ROM set.

What you need
-------------
- simpsons.zip, MAME's ROM set of The Simpsons (4 Players World, set 1), as MAME has it.

Straight to the card
--------------------
The folder anbernic is the card's Roms/PORTS as it is: copy what is in it into Roms/PORTS/ (The
Simpsons.sh, The Simpsons Arcade.sh and the folder simpsons; the other ports there stay as they are) and put
simpsons.zip in Roms/PORTS/simpsons/. The Simpsons and The Simpsons Arcade are then in the firmware's PORTS.
The first start takes the assets out of simpsons.zip into one file, Roms/PORTS/simpsons/simpsons.assets
(some seconds, the screen black); both games start from it from then on. Without simpsons.zip the game
says where to put it (in simpsons/simpsons.log, or simpsons-arcade.log).

With the tool
-------------
Start the tool for your computer, with a double click:

  Windows   Simpsons Anbernic.exe
  macOS     Simpsons Anbernic (macOS)    (the first time: right click, Open)
  Linux     Simpsons Anbernic (Linux)

Leave it where it is, beside the folder anbernic and font.bmp. Drop simpsons.zip on its window (or put it
beside the tool): it writes the folder PORTS with the assets in it, beside the tool or straight into the
Roms folder of your handheld's card (choose it as the output folder).

What either way makes from your ROM set is yours alone: never share it.

Playing
-------
  d-pad or left stick   move
  A                     attack, pick
  B                     jump, back in the menus
  START                 START (and pauses The Simpsons)
  SELECT                a coin, on The Simpsons Arcade (a coin and A bring Marge in)
  MENU                  leaves the game
  volume keys           the game's volume
The games keep their settings and their logs (simpsons.log, simpsons-arcade.log) in Roms/PORTS/simpsons/.

Licenses
--------
LICENSE.txt is the project's (BSD-3-Clause). licenses/ holds MAME's and ymfm's (the board's video and
sound follow their behavior and tables), SDL2's and SDL_GameControllerDB's (zlib; the handheld's games take
the firmware's SDL2) and Kenney's (CC0: the letters, the controls' pictures).

The Simpsons is a trademark of its owner, and the arcade game is Konami's. This project is not affiliated
with or endorsed by them, nor by Anbernic.
