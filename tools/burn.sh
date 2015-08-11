#!/bin/bash

#Burning program into the avr32 card

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

if [ ! -r "$WORKING_PATH/$1.hex" ]; then
	echo "You program doesn't exist"
	exit
fi


#flip
echo -e "Checking FLIP \n"
while ! type batchisp3 >/dev/null 2>&1
do
	echo -e "FLIP not install or not in PATH"
	echo -e "choose : \n\tIntall flip (i) \n\tDefine FLIP_BASE and add to PATH (d) \n\texit (*) ?"
	read -n 1 c
	
	case "$c" in
		"i") 
			echo " - installing"
			FLIPBASE=/opt/flip
			curl http://www.atmel.com/images/flip_linux_3-2-1.tgz | sudo tar --strip-component=1 -xvz -C $FLIPBASE
			echo "Intalled into $FLIP_BASE !!!"
			export FLIP_BASE=$FLIPBASE
			echo FLIP_BASE=$FLIP_BASE
			export PATH=$FLIP_BASE/bin:$PATH
			echo PATH=$PATH
			echo "export FLIP_BASE=$FLIPBASE" >> ~/.bash_rc
			echo "export FLIP_HOME=$FLIPBASE/bin" >> ~/.bash_rc
			echo "export PATH=$FLIP_BASE/bin:$PATH" >> ~/.bash_rc
			source ~/.bash_rc
			;;
		"d")
			echo " - defining FLIP_BASE"
			echo "Current FLIP_BASE=$FLIP_BASE"
			while [ "$flip" != "y" ] && [ "$flip" != "n" ]
			do
				echo "Keep it ? (y or no)"
				read -n 1 flip
				case "$flip" in
					"y")
						continue
					;;
					"n")
						read FLIPBASE
					;;
				esac
			done
			;;
		*)
			echo " - exiting"
			exit
			;;
	esac
done
echo "FLIP ok"

echo -e "\nExporting env variables\n"


export WORKING_PATH=$WORKING_PATH
echo WORKING_PATH=$WORKING_PATH
if [ -z $FLIP_BASE ]; then
	export FLIP_BASE=$FLIPBASE
	echo FLIP_BASE=$FLIP_BASE
	export PATH=$FLIP_BASE/bin:$PATH
	echo PATH=$PATH
	echo "export FLIP_BASE=$FLIPBASE" >> ~/.bash_rc
	echo "export FLIP_HOME=$FLIPBASE/bin" >> ~/.bash_rc
	echo "export PATH=$FLIP_BASE/bin:$PATH" >> ~/.bash_rc
	source ~/.bash_rc
fi


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
	batchisp3.sh -device AT32UC3A0512 -hardware usb -operation onfail abort memory flash erase F laodbuffer $WORKING_PATH/$1.hex program verify start reset 0
	echo -e "\n"
done
