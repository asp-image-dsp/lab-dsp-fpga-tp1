::-------------------------------------------------------------------
:: file           clean.bat
:: version        v1.1.1
:: author         Lucas A. Kammann
:: date           20210814
:: description    Clean the build directory
::-------------------------------------------------------------------

:: Move to the build/ directory to save all outputs
cd ..
mkdir build
cd build

:: Execute the assembly compiler
@echo off
cls
del *.cld
del *.lst
del *.cln
del *.lod

:: Return to the corresponding folder
cd ..
cd compiler