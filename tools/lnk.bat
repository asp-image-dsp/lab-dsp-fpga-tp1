::-------------------------------------------------------------------
:: file           lnk.bat
:: version        v1.1.1
:: author         Lucas A. Kammann
:: date           20210814
:: description    Run the linker for Motorola's DSP 56000
::-------------------------------------------------------------------

:: Move to the build/ directory to save all outputs
cd ..
mkdir build
cd build

:: Linker
:: Generate an object with COFF format (binary)
"..\compiler\dsplnk.exe" ..\src\%1

:: Translate the COFF format to LOD format (ASCII)
"..\compiler\cldlod.exe" ..\build\%1.cld > "%1.lod"

:: Return to the corresponding folder
cd ..
cd compiler