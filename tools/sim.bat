::-------------------------------------------------------------------
:: file           sim.bat
:: version        v1.1.1
:: author         Lucas A. Kammann
:: date           20210814
:: description    Run the Motorola DSP 56000 simulator
::-------------------------------------------------------------------
@echo off

:: Move to the build/ directory to save all outputs
cd ..
mkdir build
cd build

:: Find any macro with the script's first parameter as name
:: and then use it
if exist %1.mmm goto HAS_MACRO

:: Generates a file with the given macro to initiate the simulator
echo Generating macro...
echo load %1.cld > %1.tmp
echo. >> %1.tmp
goto SIMULATOR

:HAS_MACRO
copy %1.mmm %1.tmp

:SIMULATOR
echo Running simulator sim56000
"..\compiler\sim56000.exe" %1.tmp
del %1.tmp

:: Return to the corresponding folder
cd ..
cd compiler