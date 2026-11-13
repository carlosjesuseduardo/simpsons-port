# SDL_GameControllerDB

`gamecontrollerdb.txt` is [SDL_GameControllerDB](https://github.com/mdqinc/SDL_GameControllerDB)'s file at commit
`07af1d50187e94c861c9f9f5d8a55b148f11306b` (07-11-2026), as it is, with its `LICENSE` (zlib License): the community's
mappings of game controllers, which SDL does not know of all by itself (a player's report on tmnt-port: on an Nvidia
Shield a PS4 controller did not move the cursor, and in a game neither it nor an Xbox controller moved the turtle).

The window port's games on the computers and Android build their platform's lines into the executable
(`embed.cmake`, CMakeLists.txt's `simpsons_controller_db`), and `ports/sdl2/main.c` gives them to SDL as it starts,
with the environment's `SDL_GAMECONTROLLERCONFIG` and the player's own `gamecontrollerdb.txt` (beside the game, or in
the folder of what the game keeps). The Switch and the Anbernic keep their own controls. To take a newer one: the
same file from the repository's master, and its commit here.
