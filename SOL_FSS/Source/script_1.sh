#!/bin/bash
./client -f mysocket -d dir  -w Files -R n=5  -p
./client -f mysocket -d dir  -h 
./client -f mysocket -d dir  -R n=5 -p
./client -f mysocket -d dir  -R n=5 -c Files/file1.txt -p
./client -f mysocket -t 200 -p
./client -f mysocket -W Files/filecreato.txt
