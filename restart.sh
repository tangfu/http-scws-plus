#!/bin/bash

killall -9 http-scws++
./http-scws++ -p 2013 -d -c utf-8 -r ./conf/rules.utf8.ini -a ./conf/scws-dict.utf8.xdb -a ./conf/scws-extdict.utf8.txt
