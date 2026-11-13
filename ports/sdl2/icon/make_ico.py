"""
Name:        make_ico.py
Description: the Windows executables' icon from a picture of the player's own (local/icon_pc.png)
Version:     0.1.0
Created:     13-11-2026
Modified:    13-11-2026
Author:      Sebastian Toyos

Usage: python ports/sdl2/icon/make_ico.py PICTURE ICO

CMakeLists.txt runs it as it configures when this computer has a picture of the player's own for the games' icon
(SIMPSONS_PC_ICON, or local/icon_pc.png: local/ holds the player's own pictures, never committed): the picture whole
in the middle of a square, see-through around it, at every size Windows shows (16 to 256). The project has no icon of
its own yet (ports/PLAN.md): without a picture the executables have Windows' default. As tmnt-port's
tools/make_icon.py --ico.
"""
import sys
from pathlib import Path

from PIL import Image

ICO_SIZES = [16, 24, 32, 48, 64, 128, 256]


def square(picture):
    """The picture whole in the middle of a see-through square as large as its longer side."""
    picture = picture.convert("RGBA")
    side = max(picture.size)
    canvas = Image.new("RGBA", (side, side), (0, 0, 0, 0))
    canvas.paste(picture, ((side - picture.width) // 2, (side - picture.height) // 2), picture)
    return canvas


def main():
    if len(sys.argv) != 3:
        print(__doc__.split("Usage: ")[1].split("\n")[0])
        return 2
    picture = Image.open(sys.argv[1])
    out = Path(sys.argv[2])
    out.parent.mkdir(parents=True, exist_ok=True)
    largest = square(picture).resize((ICO_SIZES[-1], ICO_SIZES[-1]), Image.LANCZOS)
    largest.save(out, format="ICO", sizes=[(size, size) for size in ICO_SIZES])
    print(f"{out}: {len(ICO_SIZES)} sizes from {sys.argv[1]}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
