Ceci est un port pour executer le lab dans un environnement Linux :
Tester sous Fedora21

Une installation des outils requis est possible par :
'./SetupAvr'
Ce script telechargera les headers de AVR32 d'atmel ainsi que le toolchain puis effectuera un essais de compilation.

Vous pouvez burnez votre carte avec 
'sudo ./burn2 numduprog'
Ce script tentera d'ecrire votre programme sur votre evk1100 et installera dfu-programmer si manquant

Enfin determinez sur quel tty est votre adaptateur seriel/usb avec 'lsusb'
Puis connectez vous en seriel avec :
'sudo putty /dev/ttyUSB0'
