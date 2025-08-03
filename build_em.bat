@echo off

cls

SETLOCAL ENABLEDELAYEDEXPANSION

func_decl_gen.exe src/*

set comp=
set comp=!comp! -I"C:\Users\34687\Desktop\Dev\C\sdl"
set comp=!comp! -I"..\..\my_libs2"
set comp=!comp! -I"C:\Users\34687\Desktop\Dev\C\emsdk\upstream\emscripten\cache\sysroot\include"
@REM set comp=!comp! -I"C:\Users\34687\Desktop\Dev\C\SDL_mixer\include"
@REM set comp=!comp! -lSDL2_mixer
@REM set comp=!comp! -sUSE_SDL_MIXER=2
set comp=!comp! -lopenal
set comp=!comp! -lwebsocket.js
set comp=!comp! -lidbfs.js
set comp=!comp! -sSTACK_SIZE=1048576
set comp=!comp! --shell-file ../shell.html
set comp=!comp! -sFETCH
set comp=!comp! -Dm_emscripten
set comp=!comp! -Wbad-function-cast -Wcast-function-type
set comp=!comp! --preload-file ../shaders@shaders
set comp=!comp! --preload-file ../assets@assets
set comp=!comp! --preload-file ../src/shader_shared.h@src/shader_shared.h
set comp=!comp! -sFULL_ES3 -std=c++20 -Wno-writable-strings -sUSE_SDL=2 -sUSE_WEBGL2=1 -sALLOW_MEMORY_GROWTH
set comp=!comp! -o index.html

set debug=0
if !debug!==0 (
	set comp=!comp! -O3
	set comp=!comp! -sSAFE_HEAP=0
	set comp=!comp! -sASSERTIONS=0
) else (
	set comp=!comp! -Dm_debug
	set comp=!comp! -O0
	set comp=!comp! -sSAFE_HEAP=0
	set comp=!comp! -sASSERTIONS=1
	set comp=!comp! -gsource-map
	set comp=!comp! -fsanitize=address
)

pushd build
	call emcc ..\src\platform_emscripten.cpp ..\src\game.cpp !comp!
popd

copy build\index.html index.html > NUL
copy build\index.js index.js > NUL
copy build\index.wasm index.wasm > NUL
copy build\index.data index.data > NUL
copy build\index.wasm.map index.wasm.map > NUL