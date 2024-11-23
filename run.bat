@echo off
setlocal enabledelayedexpansion

set D="-g -DDEBUG"
set GDB=0

set DEBUG=""

for %%x in (%*) do (
    if "%%x"=="-help" (
        goto :help
    ) else if "%%x"=="g" (
        set GDB=1
        set DEBUG=%D%
        echo GDB is enabled
    ) else if "%%x"=="d" (
        set DEBUG=%D%
        echo DEBUG is enabled
    ) else (
        set PROGRAM="%%x"
    )
)

set DEBUG=%DEBUG:"=%
set VERBOSE=%VERBOSE:"=%

gcc ^
    !DEBUG! ^
    -o ./build/g.exe ^
    ./src/main.c ^
    -O0 ^
    -std=c99 ^
    -Wall ^
    -I./raylib/include/ ^
    -L./raylib/lib/ ^
    -lraylib ^
    -lopengl32 ^
    -lgdi32 ^
    -lwinmm

if not %errorlevel% equ 0 (
    echo compilation of g.exe failed
    goto :end
)

if %errorlevel% equ 0 (
    if !GDB!==1 (
        gdb .\build\g.exe
    ) else (
        .\build\g.exe !PROGRAM!
    )
)

:end