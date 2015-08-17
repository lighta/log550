#!/bin/sh
# author : lighta
# setup pour atmel sous Linux (tester avec Fedora21)
# nb les package par defaut de fedora utilise gcc 4.4.9, ceci est pour 4.4.7
# qui est actuellement le makefile que nous utilisons..
INST_PATH=/opt/avr32

sudo mkdir -p $INST_PATH
#get toolchain #https://drive.google.com/file/d/0B0ykeslA7tQ7aG51LWpFd3RqV2c/view?usp=sharing
curl -u anonymous@qk3e7ekm.ftpserver1.acanac.com:anon ftp://ftp.qk3e7ekm.ftpserver1.acanac.com/avr/avr32-gnu-toolchain-3.4.2.435-linux.any.x86_64.tar.gz | sudo tar --strip-component=1 -xvz -C $INST_PATH
#get headers #https://drive.google.com/file/d/0B0ykeslA7tQ7aVFLOVVvYmQ0WE0/view?usp=sharing
cd /tmp
curl -o headers.zip -u anonymous@qk3e7ekm.ftpserver1.acanac.com:anon ftp://ftp.qk3e7ekm.ftpserver1.acanac.com/avr/atmel-headers-6.1.3.1475.zip
unzip headers.zip
sudo mv atmel-headers-6.1.3.1475/avr32/ $INST_PATH/avr32/include
echo "export PATH=$PATH:$INST_PATH/bin" >> ~/.bashrc
source ~/.bashrc

#create symlink for eclipse as he's dumb and annoying
cd $INST_PATH
sudo ln -s avr32-gcc bin/avr-gcc
sudo ln -s avr32/ avr32/include/avr

sudo yum install dfu-programmer

#now compile something to try it out
cd /tmp
mkdir ./test
cd ./test
curl -o main.c -u anonymous@qk3e7ekm.ftpserver1.acanac.com:anon ftp://ftp.qk3e7ekm.ftpserver1.acanac.com/avr/main.c
avr32-gcc -mpart=uc3a0512 -std=gnu99 -O2 -Wall main.c -o main.elf
avr32-objcopy -O ihex -R .eeprom -R .fuse -R .lock -R .signature main.elf main.hex
echo 'Testing result'
avr32-size -B main.hex
cd ..
rm -rf ./test
