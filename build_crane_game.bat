@echo off
setlocal

cd /d "%~dp0"

set "GCC="
set "SDL_PREFIX="
set "IMAGE_ENABLED=0"
set "IMAGE_CFLAGS="
set "IMAGE_LIBS="

if defined CC (
    if exist "%CC%" set "GCC=%CC%"
    if not defined GCC (
        for /f "delims=" %%G in ('where "%CC%" 2^>nul') do if not defined GCC set "GCC=%%G"
    )
)

if not defined GCC (
    for /f "delims=" %%G in ('where gcc.exe 2^>nul') do if not defined GCC set "GCC=%%G"
)

if not defined GCC if exist "C:\msys64\ucrt64\bin\gcc.exe" set "GCC=C:\msys64\ucrt64\bin\gcc.exe"
if not defined GCC if exist "C:\msys64\mingw64\bin\gcc.exe" set "GCC=C:\msys64\mingw64\bin\gcc.exe"

if not defined GCC (
    echo Could not find gcc.exe.
    echo Install MSYS2 and the SDL2 packages, or set CC to your gcc.exe path.
    echo Expected packages: mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-SDL2 mingw-w64-ucrt-x86_64-SDL2_ttf
    echo Optional for PNG assets: mingw-w64-ucrt-x86_64-SDL2_image
    exit /b 1
)

for %%I in ("%GCC%") do set "GCC_DIR=%%~dpI"
for %%I in ("%GCC_DIR%..") do set "SDL_PREFIX=%%~fI"
set "PATH=%GCC_DIR%;%PATH%"

if not exist "%SDL_PREFIX%\include\SDL2\SDL.h" (
    echo SDL2 headers were not found at "%SDL_PREFIX%\include\SDL2".
    echo Edit SDL_PREFIX in this batch file, or install the SDL2 development package for your compiler.
    exit /b 1
)

if not exist "%SDL_PREFIX%\lib\libSDL2.dll.a" (
    echo SDL2 import libraries were not found at "%SDL_PREFIX%\lib".
    echo Edit SDL_PREFIX in this batch file, or install the SDL2 development package for your compiler.
    exit /b 1
)

if exist "%SDL_PREFIX%\include\SDL2\SDL_image.h" if exist "%SDL_PREFIX%\lib\libSDL2_image.dll.a" (
    set "IMAGE_ENABLED=1"
    set "IMAGE_CFLAGS=-DUSE_SDL_IMAGE"
    set "IMAGE_LIBS=-lSDL2_image"
)

echo Building Crane Operator: Safe Cargo Delivery...
echo Compiler: "%GCC%"
echo SDL path: "%SDL_PREFIX%"
if "%IMAGE_ENABLED%"=="1" (
    echo SDL2_image: found, PNG assets enabled
) else (
    echo SDL2_image: not found, using procedural fallback drawings
)

"%GCC%" -std=c99 -Wall -Wextra -O2 %IMAGE_CFLAGS% crane_game.c -o crane_game.exe -I"%SDL_PREFIX%\include" -L"%SDL_PREFIX%\lib" %IMAGE_LIBS% -lSDL2_ttf -lSDL2 -lm -static-libgcc
if errorlevel 1 (
    echo Build failed.
    exit /b 1
)

if "%IMAGE_ENABLED%"=="1" (
    rem SDL2_image depends on additional MSYS2 runtime DLLs. Copy the needed set so crane_game.exe can launch directly.
    for %%D in (
        SDL2.dll SDL2_ttf.dll SDL2_image.dll
        libaom.dll libavif-16.dll libbrotlicommon.dll libbrotlidec.dll libbrotlienc.dll
        libbz2-1.dll libdav1d-7.dll libdeflate.dll libfreetype-6.dll libgcc_s_seh-1.dll
        libglib-2.0-0.dll libgraphite2.dll libharfbuzz-0.dll libhwy.dll libiconv-2.dll
        libintl-8.dll libjbig-0.dll libjpeg-8.dll libjxl.dll libjxl_cms.dll liblcms2-2.dll
        libLerc.dll liblzma-5.dll libpcre2-8-0.dll libpng16-16.dll librav1e.dll
        libsharpyuv-0.dll libstdc++-6.dll libSvtAv1Enc-4.dll libtiff-6.dll libwebp-7.dll
        libwebpdemux-2.dll libwinpthread-1.dll libyuv.dll libzstd.dll zlib1.dll
    ) do (
        if exist "runtime\%%D" (
            copy /Y "runtime\%%D" "%%D" >nul
        ) else if exist "%SDL_PREFIX%\bin\%%D" (
            copy /Y "%SDL_PREFIX%\bin\%%D" "%%D" >nul
        )
    )
) else (
    rem Copy runtime fallback DLLs beside the EXE when available so the game can launch directly.
    if exist "runtime\SDL2.dll" copy /Y "runtime\SDL2.dll" "SDL2.dll" >nul
    if exist "runtime\SDL2_ttf.dll" copy /Y "runtime\SDL2_ttf.dll" "SDL2_ttf.dll" >nul
    if exist "runtime\SDL2_image.dll" copy /Y "runtime\SDL2_image.dll" "SDL2_image.dll" >nul
    if not exist "SDL2.dll" if exist "%SDL_PREFIX%\bin\SDL2.dll" copy /Y "%SDL_PREFIX%\bin\SDL2.dll" "SDL2.dll" >nul
    if not exist "SDL2_ttf.dll" if exist "%SDL_PREFIX%\bin\SDL2_ttf.dll" copy /Y "%SDL_PREFIX%\bin\SDL2_ttf.dll" "SDL2_ttf.dll" >nul
    if not exist "SDL2_image.dll" if exist "%SDL_PREFIX%\bin\SDL2_image.dll" copy /Y "%SDL_PREFIX%\bin\SDL2_image.dll" "SDL2_image.dll" >nul
)

echo.
echo Build complete: crane_game.exe
echo Run with:
echo   crane_game.exe
echo.
echo If Windows reports a missing SDL-related DLL, run:
echo   set PATH=%SDL_PREFIX%\bin;%%PATH%%
echo   crane_game.exe

endlocal
