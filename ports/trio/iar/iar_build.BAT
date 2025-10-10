@echo off
@REM call setpath

python .\gen_ewp.py

REM Create the output directory if it does not exist.
if not exist "Out\MotionPLC" mkdir Out\MotionPLC

REM invoke IAR build files to generate ELF output files
iarbuild uPy_iar.ewp Debug -parallel %NUMBER_OF_PROCESSORS%
if %errorlevel% EQU -1 goto error
if errorlevel 1 goto error

:end
exit /B 0

:error
exit /B 1
