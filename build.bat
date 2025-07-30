
@echo off

cls

if not exist build\NUL mkdir build

SETLOCAL ENABLEDELAYEDEXPANSION

func_decl_gen.exe src/*

set game_file=..\src\game.cpp
set platform_file=..\src\platform_win32.cpp
set exe_name=main

set comp=-nologo -std:c++20 -Zc:strictStrings- -Wall -FC -Gm- -GR- -EHa- -D_CRT_SECURE_NO_WARNINGS
set comp=!comp! -wd4201 -wd4820 -wd5219 -wd4365 -wd5045 -wd4191 -wd4061 -wd4505 -wd5246 -wd4711 -wd4710
set comp=!comp! -I..\..\my_libs2
@REM set comp=!comp! -fsanitize=address

set comp=!comp! -I"C:\Users\34687\Desktop\Dev\C\sdl"
set comp=!comp! -I"C:\Users\34687\Desktop\Dev\C\sdl\SDL2"
set comp=!comp! -I"C:\Users\34687\Desktop\Dev\C\SDL_mixer\include"
set comp=!comp! -I"..\src\external"
set comp=!comp! -MT
set comp=!comp! -Zi
set linker=-INCREMENTAL:NO
set linker=!linker! ..\SDL2.lib
set linker=!linker! ..\SDL2_mixer.lib
set linker=!linker! Winmm.lib User32.lib Gdi32.lib Shell32.lib Setupapi.lib Version.lib Ole32.lib Imm32.lib Advapi32.lib OleAut32.lib
set linker=!linker! -IGNORE:4099
set linker=!linker! -DYNAMICBASE:NO

set debug=2
if !debug!==0 (
	set comp=!comp! -O2
)
if !debug!==1 (
	set comp=!comp! -O2 -Dm_debug
)
if !debug!==2 (
	set comp=!comp! -Od -Dm_debug
)

set build_dll=1

pushd build

	if !build_dll!==1 (
		goto compile_dll
	) else (
		goto compile_full
	)

:compile_dll
	cl !game_file! -LD -Fe!exe_name!.dll !comp! -link !linker! -PDB:game.pdb > temp_compiler_output.txt
	if NOT !ErrorLevel! == 0 (
		type temp_compiler_output.txt
		popd
		goto fail
	)
	type temp_compiler_output.txt

	tasklist /fi "ImageName eq !exe_name!.exe" /fo csv 2>NUL | find /I "!exe_name!.exe">NUL
	if NOT !ERRORLEVEL!==0 (
		cl !platform_file! -Fe!exe_name!.exe !comp! -link !linker! -PDB:platform.pdb > temp_compiler_output.txt
		if NOT !ErrorLevel! == 0 (
			type temp_compiler_output.txt
			popd
			goto fail
		)
		type temp_compiler_output.txt
	)
	popd
	goto success


:compile_full

	cl !platform_file! !game_file! -Fe!exe_name!.exe !comp! -link !linker! -PDB:platform.pdb > temp_compiler_output.txt
	if NOT !ErrorLevel! == 0 (
		type temp_compiler_output.txt
		popd
		goto fail
	)
	type temp_compiler_output.txt
	popd
	goto success


:success
	copy build\!exe_name!.exe !exe_name!.exe > NUL
	copy build\!exe_name!.dll !exe_name!.dll > NUL
	goto end

:fail
	goto end

:end
copy build\temp_compiler_output.txt compiler_output.txt > NUL
