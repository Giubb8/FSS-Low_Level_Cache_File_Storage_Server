#!/bin/bash
echo "--------------------------------------------------------------------------------------------------------------------------------------------------------------"
echo "                                                                       SCRIPT STATISTICHE"
LOGFILE=../Source/logfile.txt
echo "LOGFILE=" "$LOGFILE"

######################################## OPEN ###############################################

#numero di operazioni open
n_open=$(grep -c "OP: OPEN" "$LOGFILE")

######################################## READ ###############################################
#numero di operazioni read lock 
n_readlock=$(grep -c "FLAG: LOCKED" "$LOGFILE")
#numero di operazioni di readn
n_readn=$(grep -c "OP: READNFILE" "$LOGFILE")
#numero di operazioni di read +readn
n_read_tot=$(grep -c "OP: READ" "$LOGFILE")
#numero di opearazioni solo read
n_read=$(echo "$n_read_tot""-""$n_readn"|bc)
#somma delle dimensioni dei file letti dalle read
sum_readedn=$(grep "READED" "$LOGFILE" | cut -d " " -f8| awk '{ SUM += $1} END { print SUM }')
#media delle dimensioni di readn 
avg_readed_readn=$(echo "$sum_readedn""/""$n_readn"|bc)

######################################## APPEND ###############################################

#numero di operazioni append
n_append=$(grep -c "OP: APPEND" "$LOGFILE")
#dimensione dei file scritti nel server 
sum_append=$(grep "WRITTEN" "$LOGFILE" | cut -d " " -f8| awk '{ SUM += $1} END { print SUM }')
#media delle dimensioni in scrittura
avg_written_append=$(echo "$sum_append""/""$n_append"|bc)

######################################## LOCK/UNLOCK ###############################################

#numero di operazioni di lock
n_lock=$(grep -c "OP: LOCK" "$LOGFILE")
#numero di operazioni di unlock
n_unlock=$(grep -c "OP: UNLOCK" "$LOGFILE")
#dimensione massima della cache 

######################################## CACHE INFO  ###############################################

dim_max_cache=$(grep "INFO" "$LOGFILE" | cut -d " " -f5)
#numero di file massimo raggiunto
num_max_file=$(grep "INFO" "$LOGFILE" | cut -d " " -f7)
#numero di rimpiazzi 
num_replace=$(grep "INFO" "$LOGFILE" | cut -d " " -f9)
#numero di connessioni in contemporanea
num_max_conn=$(grep "INFO" "$LOGFILE" | cut -d " " -f13)

echo ""
echo "--------------------------------------------------------------------------------------------------------------------------------------------------------------"
echo "SERVER INFO"
echo ""
echo "DIMENSIONE MASSIMA CACHE:" "$dim_max_cache"
echo "NUMERO DI FILE NELLA CACHE:" "$num_max_file"
echo "NUMERO DI RIMPIAZZI:" "$num_replace"
echo "NUMERO DI CONNESSIONI MASSIME IN CONTEMPORANEA:" "$num_max_conn"
echo ""
echo "--------------------------------------------------------------------------------------------------------------------------------------------------------------"
echo "STATISTICHE"
echo ""
echo "NUMERO DI OPERAZIONI OPEN:" "$n_open"
echo "NUMERO DI OPERAZIONI OPEN LOCK:" "$n_readlock"
echo "NUMERO DI OPERAZIONI APPEND" "$n_append"
echo "NUMERO DI OPERAZIONI READ" "$n_read"
echo "NUMERO DI OPERAZIONI READN:" "$n_readn"

echo "NUMERO DI BYTE SCRITTI DA APPEND:" "$sum_append"
echo "NUMERO DI BYTE LETTI DA READN:" "$sum_readedn"

echo "MEDIA DI BYTE SCRITTI DA APPEND:" "$avg_written_append"
echo "MEDIA DI BYTE LETTI DA READN/READ:""$avg_readed_readn"

echo "NUMERO DI LOCK:" "$n_lock"
echo "NUMERO DI UNLOCK:" "$n_unlock"
echo ""
echo "--------------------------------------------------------------------------------------------------------------------------------------------------------------"
echo "THREAD INFO"

echo ""
tids=();
declare -i t_count=0;
declare -i requests=0;
tids[t_count]=1;
prev_line=-1;
#costrutto con line 
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
echo "--------------------------------------------------------------------------------------------------------------------------------------------------------------"
