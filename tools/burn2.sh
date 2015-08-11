#!/bin/bash

#Burning program into the avr32 card
SKIP_CONFIRM=0
if [ -z $1 ]; then 
	echo "You must pass program name in argument"
	exit
fi

#precisez le path ou se trouve votre program

if [[ $2 != "" ]]; then
	WORKING_PATH=$2
else
	if [ -z $WORKING_PATH ]; then
		WORKING_PATH=.
	fi
fi

if [[ $3 == "-y" ]]; then
	SKIP_CONFIRM=1
fi

if [ ! -r "$WORKING_PATH/$1.hex" ]; then
	echo "You program doesn't exist"
	exit
fi

echo -e "Checking DFU"
while ! type dfu-programmer >/dev/null 2>&1
do
	echo -e "dfu-programmer not install or not in PATH"
	echo -e "choose : \n\tIntall dfu-programmer (i) \n\tDefine DFU_HOME and add to PATH (d) \n\texit (*) ?"
	read -n 1 c
	
	case "$c" in
		"i") 
			echo " - installing"
			#if fedora21 only for now
			sudo yum install dfu-programmer
			#building a small udev so usb could be acessible without root
			#yes this is a security hole but we don't care
			#we really only allow atmel product to be accessible with non root here so it's ok..
			cd ~
			mkdir tmp
			echo 'BUS=="usb", ACTION=="add", SYSFS{idVendor}=="03eb", ATTR{idProduct}=="*", MODE="666", GROUP="wheel"' > tmp/99-dfu_programmer.rules
			sudo cp tmp/99-dfu_programmer.rules /etc/udev/rules.d/99-dfu_programmer.rules
			rm -rf tmp
			sudo udevadm control --reload-rules
			echo 'Please disconect and reconnect usb cable to fire up udev rules'
			echo "Then enter 'dfu-programmer at32uc3a0512 get' to confirm it's working fine, if no bootloader is fine then use sudo to burn"
			# exit
		;;
		"d")
			echo " - defining DFU_HOME"
			echo "Current DFU_HOME=$DFU_HOME"
			while [ "$dfu" != "y" ] && [ "$dfu" != "n" ]
			do
				echo "Keep it ? (y or no)"
				read -n 1 flip
				case "$dfu" in
					"y")
						continue
					;;
					"n")
						read DFUHOME
					;;
				esac
			done
			export DFU_HOME=$DFUHOME
			echo DFU_HOME=$DFU_HOME
			export PATH=$DFU_HOME/bin:$PATH
			echo PATH=$PATH
		;;
		*)
			echo " - exiting"
			exit
		;;
	esac
done
echo "DFU ok"

#we only add them once program is confirmed present
if [ $DFU_HOME ]; then 
	echo "Adding paths permanently into bash_rc"
	echo "export DFU_HOME=$DFU_HOME" >> ~/.bash_rc
	echo "export PATH=$DFU_HOME/bin:$PATH" >> ~/.bash_rc
	source ~/.bash_rc
fi

if [[ $SKIP_CONFIRM != "1" ]]; then
	while [ "$burn" != "q" ]
	do
		
		echo -e "\n Ready to burn program ? (y / n or q(uit))"
		read -n 1 burn
		case "$burn" in
			"n")
				continue
			;;
			"q")
				exit
			;;
		esac
		echo "--------------BURNING-----------------"
		lsusb
		dfu-programmer at32uc3a0512 get && dfu-programmer at32uc3a0512 erase && dfu-programmer at32uc3a0512 flash $WORKING_PATH/$1.hex --suppress-bootloader-mem && dfu-programmer at32uc3a0512 start
		echo -e "\n"
	done
else #from deploy cmd
	echo "--------------BURNING-----------------"
	lsusb
	dfu-programmer at32uc3a0512 get && dfu-programmer at32uc3a0512 erase && dfu-programmer at32uc3a0512 flash $WORKING_PATH/$1.hex --suppress-bootloader-mem && dfu-programmer at32uc3a0512 start
	echo -e "\n"
fi
