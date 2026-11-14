# Name:        anbernic.cmake
# Description: the build for Anbernic's H700 handhelds (the RG40XX H...) on their stock firmware: The Simpsons and
#              The Simpsons Arcade, as Roms/PORTS' The Simpsons Arcade Game.sh and The Simpsons Arcade.sh and their folder
#              simpsons/
# Version:     0.1.0
# Created:     13-11-2026
# Modified:    13-11-2026
# Author:      Sebastian Toyos
#
# Included by CMakeLists.txt with SIMPSONS_ANBERNIC (and the toolchain zig_aarch64.cmake: aarch64, glibc 2.35), as
# tmnt-port's. Both games are the computers' builds with their picture fixed (SIMPSONS_HANDHELD: the whole screen,
# 640 x 480, the game's 4:3 picture; no VIDEO nor CONTROLS: the handheld's buttons as a game controller's), the
# handheld's MENU and volume keys (SIMPSONS_HANDHELD_KEYS), and with the firmware's SDL2: the executables are linked
# against a stand-in of its name (sdl2_stub.c) and SDL's headers (SIMPSONS_SDL2_INCLUDE, by default the development
# package in build/downloads/sdl2). The build is PORTS's files without the assets, in SIMPSONS_BIN_DIR/anbernic: the
# handheld takes the assets out of the player's simpsons.zip put in PORTS/simpsons/ at its first start (main.c's
# device_assets), or Simpsons Anbernic (ports/anbernic/maker), on the player's computer, writes PORTS from them and the
# player's ROM set (one assets pack in simpsons/, both games').
#
# Usage: cmake -S . -B build/anbernic -G Ninja -DCMAKE_TOOLCHAIN_FILE=ports/anbernic/zig_aarch64.cmake
#        -DSIMPSONS_ANBERNIC=ON -DSIMPSONS_REFERENCE_TOOLS=OFF [-DSIMPSONS_BIN_DIR=...]
#        cmake --build build/anbernic --target simpsons_anbernic_games --parallel 16

target_compile_definitions(simpsons_console_program PUBLIC SIMPSONS_HANDHELD SIMPSONS_HANDHELD_KEYS)

set(SIMPSONS_SDL2_INCLUDE "" CACHE PATH "SDL2's headers (the folder with SDL.h)")
if(NOT SIMPSONS_SDL2_INCLUDE)
	file(GLOB SIMPSONS_SDL2_INCLUDES ${CMAKE_SOURCE_DIR}/build/downloads/sdl2/SDL2-*/include)
	list(GET SIMPSONS_SDL2_INCLUDES 0 SIMPSONS_SDL2_INCLUDE)
endif()
if(NOT EXISTS ${SIMPSONS_SDL2_INCLUDE}/SDL.h)
	message(FATAL_ERROR "No SDL.h in SIMPSONS_SDL2_INCLUDE (${SIMPSONS_SDL2_INCLUDE})")
endif()

# the stand-in for libSDL2-2.0.so.0, beside the build's own files (never in the package)
add_library(SDL2_device SHARED ports/anbernic/sdl2_stub.c)
set_target_properties(SDL2_device PROPERTIES OUTPUT_NAME SDL2-2.0 SUFFIX ".so.0" NO_SONAME ON
	LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/sdl2_device)
target_link_options(SDL2_device PRIVATE -Wl,-soname,libSDL2-2.0.so.0)

set(SIMPSONS_ANBERNIC_OUT ${SIMPSONS_BIN_DIR}/anbernic)    # (PORTS's files, beside Simpsons Anbernic)

# A game of the handheld: its executable in the folder simpsons ('name', the window port's main.c and controls.c)
function(simpsons_anbernic_game target name program)
	add_executable(${target} ports/sdl2/main.c ports/sdl2/controls.c)
	target_include_directories(${target} PRIVATE ports/sdl2 ${SIMPSONS_SDL2_INCLUDE})
	# (the game's own pack and ROM set reader, simpsons_assets_pack in 'program': the assets out of the player's
	# simpsons.zip in PORTS/simpsons/ at the first start, for the release's package, which carries no pack)
	target_link_libraries(${target} PRIVATE ${program} SDL2_device m)
	target_compile_definitions(${target} PRIVATE SIMPSONS_HANDHELD SIMPSONS_HANDHELD_KEYS)
	set_target_properties(${target} PROPERTIES OUTPUT_NAME ${name}
		RUNTIME_OUTPUT_DIRECTORY ${SIMPSONS_ANBERNIC_OUT}/simpsons
		SKIP_BUILD_RPATH ON)    # (no path of this computer's in it: the handheld's loader finds its SDL2)
	foreach(config IN ITEMS _DEBUG _RELEASE _RELWITHDEBINFO _MINSIZEREL)
		set_target_properties(${target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY${config} ${SIMPSONS_ANBERNIC_OUT}/simpsons)
	endforeach()
endfunction()

simpsons_anbernic_game(simpsons_anbernic "The Simpsons Arcade Game" simpsons_console_game)
simpsons_anbernic_game(simpsons_arcade_anbernic "The Simpsons Arcade" ${SIMPSONS_WINDOW_PROGRAM})

# the launchers beside the folder; in it the console's letters and CONTROLS' button pictures (font.bmp,
# input_prompts.bmp). The assets pack is the handheld's first start's, or Simpsons Anbernic's, made from the player's
# ROM set with the rest of PORTS
add_custom_target(simpsons_anbernic_games ALL
	COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/ports/anbernic/The Simpsons Arcade Game.sh"
		"${CMAKE_SOURCE_DIR}/ports/anbernic/The Simpsons Arcade.sh" ${SIMPSONS_ANBERNIC_OUT}
	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/ports/sdl2/font/font.bmp
		${CMAKE_SOURCE_DIR}/ports/sdl2/prompts/input_prompts.bmp ${SIMPSONS_ANBERNIC_OUT}/simpsons
	DEPENDS simpsons_anbernic simpsons_arcade_anbernic
	COMMENT "The Simpsons' and The Simpsons Arcade's PORTS files: ${SIMPSONS_ANBERNIC_OUT}"
	VERBATIM)
