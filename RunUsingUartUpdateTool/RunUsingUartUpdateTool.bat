@echo off

rem UartUpdateTool -help

@set port=1
@set baudrate=115200

echo. 
echo Note:
echo 1. Close any terminal connection.
echo 2. Set BMC into FUP mode and issue HW reset. 

:: Sync with FUP and Auto detect COM PORT and baud-rate 
Call :CheckUppComm_l

:: Load Alternative FUP to RAM and Set baud-rate to 115Kb/s 
call :ExeclCodeFromFupMode 0xC0008000 Loader_115K.bin
@set baudrate=115200
:: Sync with FUP and Auto detect COM PORT and baud-rate 
Call :CheckUppComm_l

call :ExeclCodeFromFupMode 0xFFFD0000 ..\TestMsg_Core_0.bin
rem @set baudrate=115200

echo. 
echo. 
echo Running Tera Term terminal .... 
echo. 
echo. 

start /b ..\TeraTerm\ttermpro.exe /C=%port% /BAUD=%baudrate% /W="GFX Stand-Alone Tool" 

exit 0

:: -------------------------------------- 
:CheckUppComm_l
echo. 
echo ---------------------------------------------
CheckUppComm %port% %baudrate%
if exist SerialPortNumber.txt   set /p port=<SerialPortNumber.txt 
if exist SerialPortNumber.txt   del SerialPortNumber.txt
if exist SerialPortBaudRate.txt set /p baudrate=<SerialPortBaudRate.txt
if exist SerialPortBaudRate.txt del SerialPortBaudRate.txt
echo.
echo Serial Port settings:  COM%port%; %baudrate% bps
echo ---------------------------------------------
echo.
goto:EOF
rem exit /b
:: --------------------------------------

:: -------------------------------------- 
:ExeclCodeFromFupMode
UartUpdateTool -port %port% -baudrate %baudrate% -delay 0 -opr 2 -addr %1 -file %2
UartUpdateTool -port %port% -baudrate %baudrate% -delay 0 -opr 6 -addr %1  
goto:EOF 
rem exit /b
:: -------------------------------------- 

