#!/bin/bash
echo "--------------------------------------------------------------------------------------------------------------------------------------------------------------"
echo "                                                                       SCRIPT STATISTICHE"
echo " "
LOGFILE=./logfile.txt
echo "LOGFILE=" "$LOGFILE"

#numero di operazioni open
n_open=$(grep -c "OP: OPEN" "$LOGFILE")
#numero di operazioni read lock 
n_lock=$(grep -c "FLAG: LOCKED" "$LOGFILE")

#numero di operazioni di readn
n_readn=$(grep -c "OP: READNFILE" "$LOGFILE")
#somma delle dimensioni dei file letti da readn 
sum_readedn=$(grep "READED" "$LOGFILE" | cut -d " " -f8| awk '{ SUM += $1} END { print SUM }')
#media delle dimensioni di readn 
avg_readed_readn=$(echo "$sum_readedn""/""$n_readn"|bc)
#numero di operazioni append
n_append=$(grep -c "OP: APPEND" "$LOGFILE")
#dimensione dei file scritti nel server 
sum_append=$(grep "WRITTEN" "$LOGFILE" | cut -d " " -f8| awk '{ SUM += $1} END { print SUM }')
#media delle dimensioni di readn 
avg_written_append=$(echo "$sum_append""/""$n_append"|bc)
echo "NUMERO DI BYTE SCRITTI DA APPEND" "$sum_append"
echo "MEDIA DI BYTE SCRITTI DA APPEND" "$avg_written_append"
echo "NUMERO DI BYTE LETTI DA READN" "$sum_readedn"
echo "MEDIA BYTE LETTI DA READN ""$avg_readed_readn"
echo "NUMERO DI OPERAZIONI DI READ:" "$n_readn"
echo "NUMERO DI OPERAZIONI DI OPEN:" "$n_open"
echo "NUMERO DI OPERAZIONI DI OPEN LOCK:" "$n_lock"

tids=();
declare -i t_count=0;
declare -i requests=0;
tids[t_count]=1;
prev_line=-1;
while read -r line;do
    #echo $line
    if [ "${line}" == "${prev_line}" ]; then
        tids[t_count]=$((tids[t_count]+1))
    else
        if [ "${prev_line}" -ne -1 ]; then
            echo "thread" "$prev_line" "ha gestito " ${tids[t_count]} " richieste."
            requests=$((requests+tids[t_count]))
            t_count=$((t_count+1));
            tids[t_count]=1;
        fi
    fi
    prev_line=$line;
done<<< "$(grep "THREAD" "$LOGFILE"| cut -d " " -f2 |sort -g)"
            echo "thread" "$prev_line" "ha gestito " ${tids[t_count]} " richieste."
