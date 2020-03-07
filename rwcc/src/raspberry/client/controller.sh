#!/bin/bash

        echo "lampa controller..."
        echo "-------------------------" >>/home/pi/log/lamp_controller.log
        echo `date` >> /home/pi/log/lamp_controller.log
        echo "starting lampa controller" >>/home/pi/log/lamp_controller.log
        echo "-------------------------" >>/home/pi/log/lamp_controller.log
	plink -serial -sercfg 115200,8,n,1,X /dev/ttyUSB0 -v &>>/home/pi/log/lamp_controller.log &
	plink_pid=$!
	sleep 3
	kill -TERM $plink_pid
	sleep 1
        chown pi:pi /home/pi/log/lamp_controller.log 
        sudo -u pi /bin/bash -c "/home/pi/lamp/rwcc/src/raspberry/client/lampa &>>/home/pi/log/lamp_controller.log &"
        echo "ok"

