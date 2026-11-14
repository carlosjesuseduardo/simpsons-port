#!/bin/sh
# The Simpsons Arcade Game for Anbernic's H700 handhelds (stock firmware): the arcade game in the folder simpsons
# beside this script, with the firmware's SDL2; what it says goes to simpsons/simpsons-arcade.log
HERE=$(cd "$(dirname "$0")" && pwd)
cd "$HERE/simpsons" || exit 1
chmod +x "./The Simpsons Arcade Game" 2>/dev/null
"./The Simpsons Arcade Game" > simpsons-arcade.log 2>&1
sync
