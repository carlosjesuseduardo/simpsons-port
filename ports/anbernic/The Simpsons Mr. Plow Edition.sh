#!/bin/sh
# The Simpsons Mr. Plow Edition for Anbernic's H700 handhelds (stock firmware): the console version in the folder
# simpsons beside this script, with the firmware's SDL2; what it says goes to simpsons/simpsons.log
HERE=$(cd "$(dirname "$0")" && pwd)
cd "$HERE/simpsons" || exit 1
chmod +x "./The Simpsons Mr. Plow Edition" 2>/dev/null
"./The Simpsons Mr. Plow Edition" > simpsons.log 2>&1
sync
