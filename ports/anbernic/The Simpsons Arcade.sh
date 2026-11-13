#!/bin/sh
# The Simpsons Arcade for Anbernic's H700 handhelds (stock firmware): the arcade game in the folder simpsons beside
# this script, with the firmware's SDL2; what it says goes to simpsons/simpsons-arcade.log
HERE=$(cd "$(dirname "$0")" && pwd)
cd "$HERE/simpsons" || exit 1
chmod +x "./The Simpsons Arcade" 2>/dev/null
"./The Simpsons Arcade" > simpsons-arcade.log 2>&1
sync
