#!/bin/bash
# Name:        uninstall.sh
# Description: takes out what install.sh put in: the games' Steam shortcuts, their .desktop entries and, asked first,
#              their folder in ~/Games; never the saves (~/.local/share/<the game>)
# Version:     0.1.0
# Created:     13-11-2026
# Modified:    13-11-2026
# Author:      Sebastian Toyos
#
# license:BSD-3-Clause
# The same file in each port's Linux package (ports/linux/), run from the installed folder or the package's.
set -u

HERE="$(cd "$(dirname "$(readlink -f "$0")")" && pwd)"
NAME="$(basename "$HERE")"
DEST="$HOME/Games/$NAME"
APPLICATIONS="$HOME/.local/share/applications"
MARK="X-Installed-By=$NAME install.sh"
TOOL="$HERE/steam_shortcuts.py"
[ -f "$TOOL" ] || TOOL="$DEST/steam_shortcuts.py"

say() {
	if [ -t 1 ]; then
		echo "$1"
	elif command -v zenity >/dev/null 2>&1; then
		zenity --info --no-wrap --title="$NAME" --text="$1" 2>/dev/null
	elif command -v kdialog >/dev/null 2>&1; then
		kdialog --title "$NAME" --msgbox "$1" 2>/dev/null
	else
		echo "$1"
	fi
}

ask() {
	if [ -t 0 ]; then
		read -r -p "$1 [y/N] " answer
		[ "$answer" = y ] || [ "$answer" = Y ]
	elif command -v zenity >/dev/null 2>&1; then
		zenity --question --no-wrap --title="$NAME" --text="$1" 2>/dev/null
	elif command -v kdialog >/dev/null 2>&1; then
		kdialog --title "$NAME" --yesno "$1" 2>/dev/null
	else
		return 1
	fi
}

# The Steam shortcuts of the folder's games, Steam closed meanwhile
restart=""
if pgrep -x steam >/dev/null 2>&1 && ask "Steam is open. Close it to take the games out of it (it starts again after)?"; then
	steam -shutdown >/dev/null 2>&1 &
	for _ in $(seq 1 60); do
		pgrep -x steam >/dev/null 2>&1 || break
		sleep 1
	done
	restart=yes
fi
if ! pgrep -x steam >/dev/null 2>&1 && [ -f "$TOOL" ]; then
	for steam in "$HOME/.local/share/Steam" "$HOME/.steam/steam" "$HOME/.var/app/com.valvesoftware.Steam/.local/share/Steam"; do
		for vdf in "$steam"/userdata/*/config/shortcuts.vdf; do
			[ -f "$vdf" ] && python3 "$TOOL" remove "$vdf" "$DEST" >/dev/null
		done
	done
	[ -n "$restart" ] && (nohup steam >/dev/null 2>&1 &)
fi

# The .desktop entries install.sh wrote
for entry in "$APPLICATIONS"/*.desktop; do
	[ -f "$entry" ] && grep -qxF "$MARK" "$entry" && rm -f "$entry"
done

# The folder, asked first
if [ -d "$DEST" ] && ask "Delete $DEST (the games and the ROM set there; the saves stay)?"; then
	rm -rf "$DEST"
fi
say "The games are out of the applications menu and Steam."
