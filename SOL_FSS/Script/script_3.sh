#!/bin/bash
echo "--------------------------------------------------------------------------------------------------------------------------------------------------------------"
echo "                                                                              TEST 3"
echo " "
echo " Server Avviato "
./server ../Config_files/config_test3.txt  &

# server pid
SERVER_PID=$!
export SERVER_PID
echo "server pid ""$SERVER_PID"

# faccio partire gli script per generare i client
array_id=()
chmod +x ./script_3supp.sh

for i in {1..10}; do
	bash -c './script_3supp.sh' &
	array_id+=($!)
	sleep 0.1
done

# Aspetto 30 secondi
sleep 30s

# stop dei processi client 
for i in "${array_id[@]}"; do
	kill "${i}"
	wait "${i}" 2>/dev/null
done


kill -2 ${SERVER_PID}
wait $SERVER_PID

killall -q ./client #chiudo eventuali processi rimasti

echo "                                                                              FINE TEST 3"
echo "--------------------------------------------------------------------------------------------------------------------------------------------------------------"
