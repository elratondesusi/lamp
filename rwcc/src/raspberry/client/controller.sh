#!/bin/bash

        echo "lampa controller..."
        echo "-------------------------" >>/home/pi/log/lamp_controller.log
        echo `date` >> /home/pi/log/lamp_controller.log
        echo "starting lampa controller" >>/home/pi/log/lamp_controller.log
        echo "-------------------------" >>/home/pi/log/lamp_controller.log
	plink -serial -sercfg 115200,8,n,1,X /dev/lamp -v &>>/home/pi/log/lamp_controller.log &
	plink_pid=$!
	sleep 1
	kill -TERM $plink_pid
	echo "bind BT button..."
	rfcomm bind /dev/rfcomm0 E1:A0:02:78:90:DF &>>/home/pi/log/lamp_controller.log
	sleep 2
	echo "add SP channel to BT button..."
	sdptool add --channel=1 SP &>>/home/pi/log/lamp_controller.log
	echo "configuring serial port..."
	plink -serial -sercfg 9600,8,n,1,X /dev/rfcomm0 -v &>>/home/pi/log/lamp_controller.log &
	plink_pid=$!
	sleep 1
	kill -TERM $plink_pid
	echo "adding serial channel..."
	sudo sdptool add --channel=1 SP
	sleep 1
        chown pi:pi /home/pi/log/lamp_controller.log 
	echo "starting lampa..."
        sudo -u pi /bin/bash -c "/home/pi/lamp/rwcc/src/raspberry/client/lampa &>>/home/pi/log/lamp_controller.log &"
        echo "ok"

