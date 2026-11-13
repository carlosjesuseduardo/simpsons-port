"""
Name:        steam_shortcuts.py
Description: Steam's non-Steam games (config/shortcuts.vdf, Valve's binary KeyValues): read, written back byte for
             byte, a game added or the games of a folder taken out
Version:     0.1.0
Created:     13-11-2026
Modified:    13-11-2026
Author:      Sebastian Toyos

license:BSD-3-Clause
The same file in each port's Linux package (ports/linux/, beside install.sh and uninstall.sh); Python's standard
library alone (SteamOS has python3). shortcuts.vdf is a tree of keys: each a type byte, a name ending in 0, a value
(0x00 a subtree, 0x01 a string ending in 0, 0x02 a 32-bit integer, 0x07 a 64-bit one), and 0x08 ending a subtree.
Steam ROM Manager's way: the shortcuts are entries "0", "1"... of the root's "shortcuts", each with its appid (the
CRC-32 of the quoted executable and the name, top bit set), AppName, Exe, StartDir, icon and the rest Steam writes.
Steam must be closed while the file changes (install.sh closes it and starts it again).

Usage: python3 steam_shortcuts.py add FILE NAME EXE FOLDER ICON
       python3 steam_shortcuts.py remove FILE FOLDER
       python3 steam_shortcuts.py list FILE
       python3 steam_shortcuts.py same FILE      (read and written back: exit 0 when the bytes are the same)
"""
import struct
import sys
import zlib

MAP, STRING, INT32, INT64, END = 0x00, 0x01, 0x02, 0x07, 0x08


def parse(data):
    """The tree in 'data' as a list of (type, name, value), a subtree's value a list too."""
    position = 0

    def name():
        nonlocal position
        end = data.index(b"\0", position)
        text = data[position:end]
        position = end + 1
        return text

    def subtree():
        nonlocal position
        items = []
        while True:
            kind = data[position]
            position += 1
            if kind == END:
                return items
            key = name()
            if kind == MAP:
                items.append((kind, key, subtree()))
            elif kind == STRING:
                items.append((kind, key, name()))
            elif kind == INT32:
                items.append((kind, key, data[position:position + 4]))
                position += 4
            elif kind == INT64:
                items.append((kind, key, data[position:position + 8]))
                position += 8
            else:
                raise ValueError(f"shortcuts.vdf: a key of type {kind:#04x} at {position - 1}")

    tree = subtree()
    if position != len(data):
        raise ValueError("shortcuts.vdf: bytes after its end")
    return tree


def dump(tree):
    out = bytearray()
    for kind, key, value in tree:
        out.append(kind)
        out += key + b"\0"
        if kind == MAP:
            out += dump(value)
        elif kind == STRING:
            out += value + b"\0"
        else:
            out += value
    out.append(END)
    return bytes(out)


def load(path):
    try:
        with open(path, "rb") as file:
            data = file.read()
    except FileNotFoundError:
        data = b""
    return parse(data) if data else [(MAP, b"shortcuts", [])]


def shortcuts_of(tree):
    for kind, key, value in tree:
        if kind == MAP and key.lower() == b"shortcuts":
            return value
    tree.append((MAP, b"shortcuts", []))
    return tree[-1][2]


def field(entry, wanted):
    for kind, key, value in entry:
        if key.lower() == wanted.lower():
            return value
    return None


def appid(exe, name):
    return (zlib.crc32((exe + name).encode("utf-8")) | 0x80000000) & 0xFFFFFFFF


def add(tree, name, exe, folder, icon):
    """The game added, unless a shortcut with its executable is there already. Whether it was added"""
    shortcuts = shortcuts_of(tree)
    quoted = f'"{exe}"'
    for _, _, entry in shortcuts:
        if field(entry, b"Exe") in (quoted.encode("utf-8"), exe.encode("utf-8")):
            return False
    number = 0
    while any(key == str(number).encode() for _, key, _ in shortcuts):
        number += 1
    entry = [
        (INT32, b"appid", struct.pack("<I", appid(quoted, name))),
        (STRING, b"AppName", name.encode("utf-8")),
        (STRING, b"Exe", quoted.encode("utf-8")),
        (STRING, b"StartDir", f'"{folder}"'.encode("utf-8")),
        (STRING, b"icon", icon.encode("utf-8")),
        (STRING, b"ShortcutPath", b""),
        (STRING, b"LaunchOptions", b""),
        (INT32, b"IsHidden", struct.pack("<I", 0)),
        (INT32, b"AllowDesktopConfig", struct.pack("<I", 1)),
        (INT32, b"AllowOverlay", struct.pack("<I", 1)),
        (INT32, b"OpenVR", struct.pack("<I", 0)),
        (INT32, b"Devkit", struct.pack("<I", 0)),
        (STRING, b"DevkitGameID", b""),
        (INT32, b"DevkitOverrideAppID", struct.pack("<I", 0)),
        (INT32, b"LastPlayTime", struct.pack("<I", 0)),
        (STRING, b"FlatpakAppID", b""),
        (MAP, b"tags", []),
    ]
    shortcuts.append((MAP, str(number).encode(), entry))
    return True


def remove(tree, folder):
    """The shortcuts whose folder is 'folder' taken out, the others numbered again in order. How many"""
    shortcuts = shortcuts_of(tree)
    wanted = {f'"{folder}"'.encode("utf-8"), folder.encode("utf-8")}
    kept = [item for item in shortcuts if field(item[2], b"StartDir") not in wanted]
    removed = len(shortcuts) - len(kept)
    if removed:
        shortcuts[:] = [(kind, str(number).encode(), value) for number, (kind, _, value) in enumerate(kept)]
    return removed


def save(path, tree):
    with open(path, "wb") as file:
        file.write(dump(tree))


def main(arguments):
    if len(arguments) >= 2 and arguments[0] == "same":
        with open(arguments[1], "rb") as file:
            data = file.read()
        return 0 if dump(parse(data)) == data else 1
    if len(arguments) == 2 and arguments[0] == "list":
        for _, key, entry in shortcuts_of(load(arguments[1])):
            print(key.decode(), (field(entry, b"AppName") or b"").decode("utf-8", "replace"),
                  (field(entry, b"Exe") or b"").decode("utf-8", "replace"))
        return 0
    if len(arguments) == 6 and arguments[0] == "add":
        tree = load(arguments[1])
        if add(tree, *arguments[2:]):
            save(arguments[1], tree)
            print(f"added: {arguments[2]}")
        return 0
    if len(arguments) == 3 and arguments[0] == "remove":
        tree = load(arguments[1])
        count = remove(tree, arguments[2])
        if count:
            save(arguments[1], tree)
        print(f"removed: {count}")
        return 0
    print(__doc__.split("Usage: ")[1].split("\n\n")[0])
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
