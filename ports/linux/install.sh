#!/bin/bash
# Name:        install.sh
# Description: installs a port's Linux package for its player (the Steam Deck's desktop mode, or any Linux): the
#              folder in ~/Games, each game in the applications menu and in Steam as a non-Steam game
# Version:     0.1.0
# Created:     13-11-2026
# Modified:    13-11-2026
# Author:      Sebastian Toyos
#
# license:BSD-3-Clause
# The same file in each port's Linux package (ports/linux/), beside its games: it needs nothing of the port's own.
# The games are the folder's ELF executables, their icon the folder's .png, the ROM set the folder's .zip (or
# ~/Downloads/<the folder>.zip); the games keep their saves in ~/.local/share/<the game> already. It copies the folder
# to ~/Games/<the folder>, writes each game's .desktop entry in ~/.local/share/applications, and adds each game to
# every Steam user's shortcuts.vdf (steam_shortcuts.py), Steam closed meanwhile and started again. Run again, it
# copies the games again and adds nothing twice. uninstall.sh takes it out. Its messages go to a dialog (zenity or
# kdialog) when it was not run from a terminal.
set -u

HERE="$(cd "$(dirname "$(readlink -f "$0")")" && pwd)"
NAME="$(basename "$HERE")"
DEST="$HOME/Games/$NAME"
APPLICATIONS="$HOME/.local/share/applications"
MARK="X-Installed-By=$NAME install.sh"

say() {    # a message: in a dialog when there is no terminal
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

ask() {    # a yes or no question: 0 for yes
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

is_game() {    # an ELF executable
	[ -f "$1" ] && [ -x "$1" ] && [ "$(head -c 4 "$1" | od -An -c | tr -d ' ')" = '177ELF' ]
}

# The folder in ~/Games (the games again when it is there), the ROM set with it
mkdir -p "$DEST"
if [ "$HERE" != "$DEST" ]; then
	cp -a "$HERE"/. "$DEST"/ || { say "The games cannot be copied to $DEST."; exit 1; }
fi
if ! ls "$DEST"/*.zip >/dev/null 2>&1 && [ -f "$HOME/Downloads/$NAME.zip" ]; then
	cp "$HOME/Downloads/$NAME.zip" "$DEST/"
fi
ICON="$(ls "$DEST"/*.png 2>/dev/null | head -n 1)"

GAMES=()
for file in "$DEST"/*; do
	is_game "$file" && GAMES+=("$file")
done
if [ ${#GAMES[@]} -eq 0 ]; then
	say "No game in $DEST."
	exit 1
fi

# Each game in the applications menu
mkdir -p "$APPLICATIONS"
for game in "${GAMES[@]}"; do
	title="$(basename "$game")"
	entry="$APPLICATIONS/$(echo "$title" | tr ' ' '-' | tr -cd '[:alnum:]-_.').desktop"
	{
		echo "[Desktop Entry]"
		echo "Type=Application"
		echo "Name=$title"
		echo "Exec=\"$game\""
		echo "Path=$DEST"
		[ -n "$ICON" ] && echo "Icon=$ICON"
		echo "Categories=Game;"
		echo "Terminal=false"
		echo "$MARK"
	} > "$entry"
done

# Each game in Steam, as a non-Steam game, in every Steam user's shortcuts.vdf (Steam closed meanwhile)
CONFIGS=()
for steam in "$HOME/.local/share/Steam" "$HOME/.steam/steam" "$HOME/.var/app/com.valvesoftware.Steam/.local/share/Steam"; do
	for config in "$steam"/userdata/*/config; do
		[ -d "$config" ] || continue
		real="$(readlink -f "$config")"
		case " ${CONFIGS[*]-} " in *" $real "*) continue ;; esac
		CONFIGS+=("$real")
	done
done
STEAM_ADDED=""
if [ ${#CONFIGS[@]} -gt 0 ]; then
	restart=""
	if pgrep -x steam >/dev/null 2>&1; then
		if ask "Steam is open. Close it to add the games to it (it starts again after)?"; then
			steam -shutdown >/dev/null 2>&1 &
			for _ in $(seq 1 60); do
				pgrep -x steam >/dev/null 2>&1 || break
				sleep 1
			done
			restart=yes
		fi
	fi
	if ! pgrep -x steam >/dev/null 2>&1; then
		for config in "${CONFIGS[@]}"; do
			for game in "${GAMES[@]}"; do
				python3 "$DEST/steam_shortcuts.py" add "$config/shortcuts.vdf" "$(basename "$game")" "$game" "$DEST" \
					"${ICON:-}" >/dev/null || say "Steam's shortcuts in $config could not be written."
			done
		done
		STEAM_ADDED=yes
		[ -n "$restart" ] && (nohup steam >/dev/null 2>&1 &)
	fi
fi

message="Installed in $DEST: ${#GAMES[@]} games in the applications menu"
[ -n "$STEAM_ADDED" ] && message="$message and in Steam (Library, non-Steam games)"
ls "$DEST"/*.zip >/dev/null 2>&1 || message="$message. Put the ROM set in $DEST before playing"
say "$message."
