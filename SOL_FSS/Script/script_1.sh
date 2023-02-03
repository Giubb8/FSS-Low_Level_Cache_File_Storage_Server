#!/bin/bash
echo "--------------------------------------------------------------------------------------------------------------------------------------------------------------"
echo "                                                                              TEST 1"
echo " "

echo " Server Avviato "
valgrind -s --leak-check=full  ./server ../Config_files/config_test1.txt &
# server pid
SERVER_PID=$!
export SERVER_PID


#sleep 1s
echo " Avviando i Client"
#sleep 1s

./client -f mysocket -d dir  -w Files -R n=5  -p
./client -f mysocket -d dir  -h 
./client -f mysocket -d dir  -R n=5 -p
./client -f mysocket -d dir  -R n=0 -c Files/file1.txt -p
./client -f mysocket -t 200 -p
./client -f mysocket -W Files/filecreato.txt -l filecreato.txt -r filecreato.txt -u filecreato.txt -p
./client -f mysocket  -p 

echo ""
echo "Server in Chiusura"
#sleep 1s


kill -s SIGHUP $SERVER_PID
wait $SERVER_PID
echo "                                                                              FINE TEST 1"
echo "--------------------------------------------------------------------------------------------------------------------------------------------------------------"
