#!/bin/bash

        echo "lampa server..."
        echo "-----------------------" >>/home/lampa/log/node.log
        echo `date` >>/home/lampa/log/node.log
        echo "starting node for lampa" >>/home/lampa/log/node.log
        echo "-----------------------" >>/home/lampa/log/node.log
        chown lampa:lampa /home/lampa/log/node.log 
        sudo -u lampa -i /bin/bash -c "/usr/bin/node /home/lampa/lamp/rwcc/src/server/server.js &>>/home/lampa/log/node.log &"
        echo "ok"

