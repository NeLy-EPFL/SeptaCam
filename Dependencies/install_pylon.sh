#!/bin/bash

mkdir /pylon 
apt-get update
apt-get install sudo=1.8.16-0ubuntu1.5 -y
cp /NIR_Imaging/Dependencies/pylon-5.0.11.10914-x86_64.tar /pylon
cd /pylon
tar -xf pylon-5.0.11.10914-x86_64.tar 
sudo tar -C /opt -xzf pylon-5.0.11.10914-x86_64/pylonSDK*.tar.gz
cd /pylon/pylon-5.0.11.10914-x86_64/
apt-get update
apt-get install expect=5.45-7 -y
cp /NIR_Imaging/Dependencies/setup-usb-auto.sh . 
mkdir /etc/udev/rules.d/ 
chmod +x setup-usb-auto.sh
./setup-usb-auto.sh