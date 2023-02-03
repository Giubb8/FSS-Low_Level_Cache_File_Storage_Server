#!/bin/bash
echo "--------------------------------------------------------------------------------------------------------------------------------------------------------------"
echo "                                                                              TEST 2"
echo " "

echo " Server Avviato "
./server ../Config_files/config_test2.txt &
# server pid
SERVER_PID=$!
export SERVER_PID

#sleep 1s

echo " Client Avviati "

#sleep 1s
./client -f mysocket -D dir_replace  -w Files/dirtovisit1 -p
./client -f mysocket -D dir_replace  -w Files -p 
./client -f mysocket -D dir_replace_2 -w Files/dirtovisit1 -p

echo "Server in Chiusura"
#sleep 1s

echo ""
kill -s SIGHUP $SERVER_PID
wait $SERVER_PID
echo "                                                                              FINE TEST 2"
echo "--------------------------------------------------------------------------------------------------------------------------------------------------------------"
