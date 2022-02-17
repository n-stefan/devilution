# Devilution for DiabloBlazor

This is a fork of Devilution for Web (original readme below), made for [DiabloBlazor](https://github.com/n-stefan/diabloblazor).

# Devilution for Web

This is a fork of the original Devilution project, made for https://github.com/d07RiV/diabloweb

It has been stripped down from all dependencies (Storm.dll, DiabloUI.dll, DirectDraw/DirectSound and Windows), and can be built as a Windows app (using WinAPI and DirectSound) or compiled for web using Emscripten.
It should be fairly easy to port to other platforms as all non-portable functionality is isolated to a few files (search for EMSCRIPTEN in code). Correct behavior is not guaranteed, as I had to modify many parts of the code in the process. Network functionality is completely broken currently, but I plan on attempting to restore it.

To run it normally, simply build the solution (tested in VS2017, but should work with other versions as well) and set the starting directory to somewhere that contains SPAWN.MPQ or DIABDAT.MPQ (depending on whether SPAWN was defined in options).

To compile it to WASM/JSCC (jscc is an extension I'm using for javascript glue files so they don't mix with regular JS in my webpack config), simply run `node build.js`, it should automatically build only the modified files. Requires [Emscripten](https://emscripten.org/docs/getting_started/downloads.html) installed.

The source code in this repository is for non-commerical use only. If you use the source code you may not charge others for access to it or any derivative work thereof.

Original readme can be found in https://github.com/diasurgical/devilution
