@echo off
setlocal EnableDelayedExpansion

:: param1: output file name 
:: param2: Core_0 or Core_1
if  -%1- == -- (set PARAM_1=TestMsg) else (set PARAM_1=%1)
if  -%2- == -- (set PARAM_2=Core_0) else (set PARAM_2=%2)


set OUTPUT_FILE=%PARAM_1%
set OUTPUT_FILE=%OUTPUT_FILE%_%PARAM_2%
set OUTPUT_DIR=output_%PARAM_2%
set LINKER_FILE="linker_%PARAM_2%.ld"

echo. & echo   Settings: 
set OUTPUT_FILE
set OUTPUT_DIR
set LINKER_FILE

:: Set path for GCC compiler 
if exist "C:\Program Files\GNU Tools ARM Embedded\4.8 2013q4" set PATH_GCC=C:\Program Files\GNU Tools ARM Embedded\4.8 2013q4
if exist "C:\Program Files (x86)\GNU Tools ARM Embedded\4.8 2013q4"  set PATH_GCC=C:\Program Files (x86)\GNU Tools ARM Embedded\4.8 2013q4
if exist "C:\Program Files (x86)\GNU Tools ARM Embedded\5.3 2016q1"  set PATH_GCC=C:\Program Files (x86)\GNU Tools ARM Embedded\5.3 2016q1
if exist "C:\Program Files (x86)\GNU Tools ARM Embedded\7 2018-q2-update" set PATH_GCC=C:\Program Files (x86)\GNU Tools ARM Embedded\7 2018-q2-update
if exist %GCC_TOOLS% set PATH_GCC=%GCC_TOOLS%
set PATH=%PATH_GCC%\bin


rem set PATH_LIB_POSTFIX=armvx
rem set CPU_TYPE=arm926ej-s

set PATH_LIB_POSTFIX=armv7-ar
set CPU_TYPE=cortex-a9

set GCC_OPT=-mcpu=%CPU_TYPE% -mlittle-endian -mthumb-interwork -marm -Wall -Os -g -D %PARAM_2% -fpie -mpic-data-is-text-relative
set GAS_OPT=-mcpu=%CPU_TYPE% -Wa,-mthumb-interwork -Wa,--fatal-warnings -Wa,--keep-locals -Wa,--gdwarf-2
set GLD_OPT=-mcpu=%CPU_TYPE% -Wl,-script=%LINKER_FILE% -Wl,-Map=%OUTPUT_DIR%\%OUTPUT_FILE%.map -nostartfiles -Wl,--warn-section-align -Wl,--gc-sections -Wl,--cref 
::-Wl,--pic-executable 


echo. & echo   Using CPU type: %CPU_TYPE%

echo. & echo   Using those include path:
echo   %PATH_GCC%\arm-none-eabi\include
echo   %PATH_GCC%\arm-none-eabi

echo. & echo   Using those lib path:
echo   %PATH_GCC%\arm-none-eabi\lib\%PATH_LIB_POSTFIX%
echo   %PATH_GCC%\lib\%PATH_LIB_POSTFIX%

echo. & echo   cleaning previous files...
if exist %OUTPUT_DIR%\*.o    del %OUTPUT_DIR%\*.o
if exist %OUTPUT_DIR%\*.d    del %OUTPUT_DIR%\*.d
if exist %OUTPUT_DIR%\*.bin  del %OUTPUT_DIR%\*.bin
if exist %OUTPUT_DIR%\*.hex  del %OUTPUT_DIR%\*.hex
if exist %OUTPUT_DIR%\*.elf  del %OUTPUT_DIR%\*.elf
if exist %OUTPUT_DIR%\*.map  del %OUTPUT_DIR%\*.map
if exist %OUTPUT_DIR%\*.disassemble  del %OUTPUT_DIR%\*.disassemble

echo. & echo   start compile ...
for %%i in (*.c) do call :compile_gcc %%i 
for %%i in (*.s) do call :compile_gas %%i 

echo. & echo   start link ...
arm-none-eabi-gcc.exe -o %OUTPUT_DIR%\%OUTPUT_FILE%.elf %OBJ_LIST% %GLD_OPT% -L"%PATH_GCC%\arm-none-eabi\lib\%PATH_LIB_POSTFIX%" -L"%PATH_GCC%\lib\%PATH_LIB_POSTFIX%"  2>&1 | Gcc2Msvc
if not exist %OUTPUT_DIR%\%OUTPUT_FILE%.elf goto error

echo. & echo   code size: 
rem arm-none-eabi-gcc-nm.exe %OUTPUT_DIR%\%OUTPUT_FILE%.elf
rem arm-none-eabi-size.exe -A -t -x %OUTPUT_DIR%\%OUTPUT_FILE%.elf 
arm-none-eabi-size.exe -B -t -x %OUTPUT_DIR%\%OUTPUT_FILE%.elf 

echo. & echo   generate BIN and HEX and disassembly files ...
arm-none-eabi-objcopy.exe --output-target binary  %OUTPUT_DIR%\%OUTPUT_FILE%.elf %OUTPUT_DIR%\%OUTPUT_FILE%.bin
arm-none-eabi-objcopy.exe --output-target ihex    %OUTPUT_DIR%\%OUTPUT_FILE%.elf %OUTPUT_DIR%\%OUTPUT_FILE%.hex
arm-none-eabi-objdump.exe --disassemble --disassembler-options reg-names-std  --source %OUTPUT_DIR%\%OUTPUT_FILE%.elf  > %OUTPUT_DIR%\%OUTPUT_FILE%.disassemble

if not exist %OUTPUT_DIR%\%OUTPUT_FILE%.bin goto error
goto pass

:error
echo. & echo.
echo   *****************
echo        ERROR
echo   *****************
pause
exit 1

:pass
echo. & echo.
echo   *****************
echo       PASS :-)
echo   *****************
pause
exit 0


:: -------------------------------------- 
:compile_gcc
echo   compiling %1:
arm-none-eabi-gcc.exe -c %1 -o %OUTPUT_DIR%\%1.o %GCC_OPT% -MMD -I"%PATH_GCC%\arm-none-eabi\include" -I"%PATH_GCC%\arm-none-eabi" 2>&1 | Gcc2Msvc
set OBJ_LIST=%OBJ_LIST% %OUTPUT_DIR%\%1.o 
goto:EOF
rem exit /b
:: --------------------------------------


:: -------------------------------------- 
:compile_gas
echo   compiling %1:
arm-none-eabi-gcc.exe -c %1 -o %OUTPUT_DIR%\%1.o %GAS_OPT% -MMD -I"%PATH_GCC%\arm-none-eabi\include" -I"%PATH_GCC%\arm-none-eabi" 2>&1 | Gcc2Msvc
set OBJ_LIST=%OBJ_LIST% %OUTPUT_DIR%\%1.o 
goto:EOF
rem exit /b
:: --------------------------------------