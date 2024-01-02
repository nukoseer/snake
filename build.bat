@echo off

IF NOT EXIST build mkdir build
pushd build

set debug_compiler_flags=-DDEBUG
set release_compiler_flags=-flto -O3
set common_compiler_flags=--target=wasm32 -nostdlib -Wall -Werror -Wno-unused-function

set debug_linker_flags=
set release_linker_flags=-Wl,--lto-O3
set common_linker_flags=-Wl,--no-entry  -Wl,--allow-undefined -Wl,--export=game_init -Wl,--export=game_key_down -Wl,--export=game_update -Wl,--export=game_render -Wl,--export=get_arena_used -Wl,--export=get_arena_size

set debug=no

if %debug%==yes (
   set common_compiler_flags=%common_compiler_flags% %debug_compiler_flags%
   set common_linker_flags=%common_linker_flags% %debug_linker_flags%
) else (
   set common_compiler_flags=%common_compiler_flags% %release_compiler_flags%
   set common_linker_flags=%common_linker_flags% %release_linker_flags%
)

clang %common_compiler_flags% %common_linker_flags% -o snake.wasm ../snake.c

popd
