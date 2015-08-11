::Burning a program into the avr32 card

::@echo off
::precisez votre path ou se trouve les bin de sortie
IF "%2"=="" (
 set WORKING_PATH=.
)
else set WORKING_PATH=%2

batchisp 2> NUL
if not %ERRORLEVEL%==9009 goto :burning

::@echo on
::add atmin flip into path (c'est de la merde mais bon)
:define_flip
IF "%FLIP_HOME%"=="" (
  echo "FLIP_HOME should be C:\Program Files (x86)\Atmel\Flip 3.4.7 by default"
  set /p FLIP_HOME="Enter FLIP_HOME (path de d'atmel flip ) : "
  goto :defined_flip
)

:defined_flip
set /p res="Using FLIP_HOME=%FLIP_HOME% (Y/N):  "
IF NOT "%res%"=="Y" (
   set /p FLIP_HOME="Enter FLIP_HOME (path de d'atmel flip ) :  "
   goto :defined_flip
)

:registring
echo Registering FLIP_HOME as %FLIP_HOME%
echo Adding flip bin to path
set path=%PATH%;%FLIP_HOME%\bin

:burning
set /p res="Gonna burn %1 into the card, continue ? (Y/N):  "
IF "%res%"=="Q" (
  exit
) 
IF "%res%"=="N" (
  goto :burning
) 

::burn the prog into card
batchisp -device AT32UC3A0512 -hardware usb -operation onfail abort memory FLASH erase F loadbuffer %WORKING_PATH%\%1.hex program verify start reset 0

goto :burning