::-------------------------------------------------------------------
:: file           all.bat
:: version        v1.1.1
:: author         Lucas A. Kammann
:: date           20210814
:: description    Run all steps of the DSP56000 toolchain
::-------------------------------------------------------------------

:: Clean the build/ directory
call clean.bat

:: Compile the assembly language and generate the object code
call asm.bat %1
IF %ERRORLEVEL% NEQ 0 GOTO LBLERR

:: Link and generate the executable file
call lnk.bat %1
IF %ERRORLEVEL% NEQ 0 GOTO LBLERR

GOTO LBLEND
:LBLERR
echo Compilation error!
:LBLEND