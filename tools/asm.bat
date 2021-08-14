::-------------------------------------------------------------------
:: file           asm.bat
:: version        v1.1.1
:: author         Lucas A. Kammann
:: date           20210814
:: description    Run the assembly compiler for Motorola's DSP 56000
::-------------------------------------------------------------------

:: Move to the build/ directory to save all outputs
@echo off
cd ..
mkdir build
cd build
@echo on

:: Execute the assembly compiler
"..\compiler\asm56000.exe" -b -l ..\src\%1.asm

:: Return to the corresponding folder
@echo off
cd ..
cd tools
@echo on