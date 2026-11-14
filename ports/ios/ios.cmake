# Name:        ios.cmake
# Description: the iOS and iPadOS build (Xcode, CMake's Xcode generator): The Simpsons and The Simpsons Arcade Game as apps
#              on SDL2's UIKit backend, for an iPhone or an iPad (arm64) or the Simulator, and an unsigned .ipa of each
# Version:     0.1.0
# Created:     13-11-2026
# Modified:    13-11-2026
# Author:      Sebastian Toyos
#
# Included by CMakeLists.txt when CMAKE_SYSTEM_NAME is iOS. Both games are Android's builds on Apple's phones and
# tablets: the screen whole (SIMPSONS_HANDHELD: no first run's window, no VSYNC nor window lines), no EXIT in The
# Simpsons' menu (SIMPSONS_SYSTEM_EXIT: the home gesture leaves it), the touch screen's controls (SIMPSONS_TOUCH,
# ports/sdl2/touch.c), The Simpsons' WIDE by the screen's shape (main.c's follow_screen), the game controllers iOS
# knows (MFi, Xbox, PlayStation, Switch Pro: SDL's MFi driver, with SDL_GameControllerDB's iOS lines). Each app reads
# its assets pack, simpsons.assets, in its Documents folder, which the Files app shows as the app's own (Info.plist.in:
# UIFileSharingEnabled, LSSupportsOpeningDocumentsInPlace); with none there it makes it there of the player's
# simpsons.zip at its first start (main.c's device_assets). Two apps, two folders: each its own copy. SDL2 from its
# source, as the releases' (SIMPSONS_SDL2_SOURCE, else SDL's repository at release-2.32.10), static, with SDL2main
# (UIKit's main, which starts the game's on the main thread: the program needs less than 32 KB of stack, ports/PLAN.md
# A1). The icon is the project's (Assets.xcassets: Homer at the window, as Android's), the launch screen black
# (LaunchScreen.storyboard), both compiled by Xcode. The apps are not signed unless SIMPSONS_IOS_TEAM names a
# development team (a Mac building for its own iPhone): the release's .ipa is unsigned, and the player signs it as it
# is installed (AltStore, SideStore, Sideloadly: docs/package/README-ios.txt).
#
# Usage (macOS, Xcode; for the Simulator, CMAKE_OSX_SYSROOT iphonesimulator, in a build folder of its own):
#   cmake -S . -B build/ios -G Xcode -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_ARCHITECTURES=arm64
#       -DCMAKE_OSX_DEPLOYMENT_TARGET=13.0 -DSIMPSONS_REFERENCE_TOOLS=OFF [-DCMAKE_OSX_SYSROOT=iphonesimulator]
#       [-DSIMPSONS_BIN_DIR=build/bin-ios] [-DSIMPSONS_IOS_TEAM=<team ID>]
#   cmake --build build/ios --config Release --target simpsons_ios_games -- CODE_SIGNING_ALLOWED=NO
# The apps: SIMPSONS_BIN_DIR/ios/The Simpsons Mr. Plow Edition.app and The Simpsons Arcade Game.app; for a device, also The Simpsons Mr. Plow Edition.ipa and
# The Simpsons Arcade Game.ipa there (each app inside Payload/, zipped).

if(NOT CMAKE_GENERATOR STREQUAL "Xcode")
	message(FATAL_ERROR "The iOS build is the Xcode generator's (-G Xcode): Xcode compiles the apps' launch screen and "
		"icon, and puts the bundles together")
endif()

set(SIMPSONS_IOS_DEFINITIONS SIMPSONS_HANDHELD SIMPSONS_SYSTEM_EXIT SIMPSONS_TOUCH)
target_compile_definitions(simpsons_console_program PUBLIC ${SIMPSONS_IOS_DEFINITIONS})

set(SIMPSONS_IOS_TEAM "" CACHE STRING "The development team that signs the apps (none: unsigned, as the release's)")
set(SIMPSONS_IOS_OUT ${SIMPSONS_BIN_DIR}/ios)
if(CMAKE_OSX_SYSROOT MATCHES "[Ss]imulator")
	set(SIMPSONS_IOS_SIMULATOR ON)
endif()

# SDL2 from its source, static, and SDL2main (src/main/uikit: main() starts UIKit, which calls the game's). No HIDAPI:
# iOS's game controllers come through Apple's GameController framework, and SDL's HIDAPI there talks Bluetooth itself,
# which needs a permission the apps do not ask for
include(FetchContent)
if(EXISTS ${SIMPSONS_SDL2_SOURCE}/CMakeLists.txt AND NOT FETCHCONTENT_SOURCE_DIR_SDL2)
	set(FETCHCONTENT_SOURCE_DIR_SDL2 ${SIMPSONS_SDL2_SOURCE})
endif()
FetchContent_Declare(SDL2 GIT_REPOSITORY https://github.com/libsdl-org/SDL.git GIT_TAG release-2.32.10 GIT_SHALLOW TRUE)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL_HIDAPI OFF CACHE BOOL "" FORCE)
set(SDL_CCACHE OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SDL2)

# the apps' resources, which Xcode copies into each bundle (compiling the launch screen and the asset catalog): the
# letters of the touch controls and the menus, CONTROLS' pictures, the launch screen, the icon
set(SIMPSONS_IOS_RESOURCES ports/sdl2/font/font.bmp ports/sdl2/prompts/input_prompts.bmp
	ports/ios/LaunchScreen.storyboard ports/ios/Assets.xcassets)
set_source_files_properties(${SIMPSONS_IOS_RESOURCES} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

set(SIMPSONS_IOS_OUTPUTS "")

# A game of iOS: its app ('target', named 'name', its bundle 'identifier'), the window port's main.c, controls.c,
# letters.c and touch.c on 'program' (whose library reads the assets pack, and the player's ROM set on the device:
# main.c's device_assets); for a device, its .ipa beside it
function(simpsons_ios_game target name identifier program)
	add_executable(${target} MACOSX_BUNDLE ports/sdl2/main.c ports/sdl2/controls.c ports/sdl2/letters.c
		ports/sdl2/touch.c ${SIMPSONS_IOS_RESOURCES})
	target_include_directories(${target} PRIVATE ports/sdl2)
	# (objc: main.c's safe area, through the Objective-C runtime; GameController and CoreHaptics as SDL's MFi driver
	# takes them, weak: an older iOS starts without what it lacks)
	target_link_libraries(${target} PRIVATE ${program} simpsons_controller_db SDL2::SDL2main SDL2::SDL2-static objc)
	target_link_options(${target} PRIVATE "SHELL:-weak_framework GameController" "SHELL:-weak_framework CoreHaptics")
	target_compile_definitions(${target} PRIVATE ${SIMPSONS_IOS_DEFINITIONS})
	set_target_properties(${target} PROPERTIES OUTPUT_NAME ${name}
		MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/ports/ios/Info.plist.in
		MACOSX_BUNDLE_BUNDLE_NAME ${name} MACOSX_BUNDLE_GUI_IDENTIFIER ${identifier}
		MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION} MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
		XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER ${identifier}
		XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"    # (iPhone and iPad)
		XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME AppIcon)
	foreach(config IN ITEMS "" _DEBUG _RELEASE _RELWITHDEBINFO _MINSIZEREL)
		set_target_properties(${target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY${config} ${SIMPSONS_IOS_OUT})
	endforeach()
	if(SIMPSONS_IOS_TEAM)
		set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_DEVELOPMENT_TEAM ${SIMPSONS_IOS_TEAM}
			XCODE_ATTRIBUTE_CODE_SIGN_STYLE Automatic XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Apple Development")
	else()
		set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED NO
			XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "")
	endif()

	set(outputs "")
	if(NOT SIMPSONS_IOS_SIMULATOR)
		# its .ipa: the app in Payload/, zipped (ditto keeps the bundle's permissions), as a sideloading tool takes it
		set(stage ${CMAKE_BINARY_DIR}/ipa/${target})
		set(ipa ${SIMPSONS_IOS_OUT}/${name}.ipa)
		add_custom_command(OUTPUT ${ipa}
			COMMAND ${CMAKE_COMMAND} -E rm -rf ${stage} ${ipa}
			COMMAND ${CMAKE_COMMAND} -E make_directory ${stage}/Payload
			COMMAND ditto $<TARGET_BUNDLE_DIR:${target}> ${stage}/Payload/${name}.app
			COMMAND ${CMAKE_COMMAND} -E chdir ${stage} ditto -c -k --keepParent Payload ${ipa}
			DEPENDS ${target}
			COMMENT "${name}.ipa (unsigned: the player's sideloading tool signs it)"
			VERBATIM)
		list(APPEND outputs ${ipa})
	endif()
	set(SIMPSONS_IOS_OUTPUTS ${SIMPSONS_IOS_OUTPUTS} ${outputs} PARENT_SCOPE)
endfunction()

simpsons_ios_game(simpsons_ios "The Simpsons Mr. Plow Edition" io.github.cheviman.simpsons simpsons_console_game)
simpsons_ios_game(simpsons_arcade_ios "The Simpsons Arcade Game" io.github.cheviman.simpsons.arcade ${SIMPSONS_WINDOW_PROGRAM})
add_custom_target(simpsons_ios_games ALL DEPENDS ${SIMPSONS_IOS_OUTPUTS})
add_dependencies(simpsons_ios_games simpsons_ios simpsons_arcade_ios)
