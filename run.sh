#!/bin/bash

PROJ_Dir=/root/doc/prog/cpp/MisakaSisters
program=$PROJ_Dir/misaka_sisters

List=$PROJ_Dir/list
ProxyList=$List/proxy/1.list

# directory to save pid file
if [ ! -d "$HOME/.misaka" ]; then
    mkdir $HOME/.misaka
else
    rm $HOME/.misaka/*
fi


# check
pid=`nohup $program check $List/check/1.list NO_CACHE $ProxyList > /dev/null & echo $!`
pids=($pid)
pidsRunning=(1)
pids[1]=`nohup $program check $List/check/2.list NO_CACHE $ProxyList > /dev/null & echo $!`
pidsRunning[1]=1
pids[2]=`nohup $program check $List/check/3.list NO_CACHE $ProxyList > /dev/null & echo $!`
pidsRunning[2]=1

## search
pids[3]=`nohup $program search $List/search/1.list $ProxyList > /dev/null & echo $!`
pidsRunning[3]=1


# Wait for all background program exit
while [ 1 ]
do
    for((i=0;i<${#pids[@]};i++))
    do
        pid=${pids[i]}
        running=${pidsRunning[i]}
        if [ $running -eq 0 ]; then
            continue
        fi

        # check pid exist
        ps -ax | awk '{ print $1 }' | grep -e "$pid"
        if [ $? -ne 0 ]; then
            pidsRunning[i]=0
            continue
        fi

        continue

        # check pid file
        pidfile="~/.misaka/$pid"
        if [ ! -e pidfile ]; then pidsRunning[i]=1; continue; fi
        arr=(`tail -n1 $pidfile`)
        if [ ${#arr[@]} -eq 0 ]; then
            # error
            # won't happen
            pidsRunning[i]=0
        else
            if [ ${arr[0]} = "QUIT" ]; then
                pidsRunning[i]=0
            elif echo ${arr[0]} | grep -q '[^0-9]'; then
                current=`date "+%Y-%m-%d %H:%M:%S"`
                timeStamp=`date -d "$current" +%s` 
                timeSpan=`expr $timeStamp -${arr[0]}`
                if [ $timeSpan -gt 500 ]; then
                    pidsRunning[i]=0
                fi
            fi
        fi
    done

    # all program quit
    count=0
    for((i=0;i<${#pidsRunning[@]};i++))
    do
        if [ ${pidsRunning[i]} -eq 0 ]; then
            ((count++))
        fi
    done
    echo "count=$count"
    if [ $count -eq ${#pidsRunning[@]} ]; then
        break
    fi

    sleep 60
done


##################################
#        export data             #
##################################

# date
DataDir=$PROJ_Dir/data
NOW=`date +%Y%m%d_%H%M%S`
DataDirForThis=$DataDir/$NOW
if [ ! -d "$DataDirForThis" ]; then
    mkdir -p $DataDirForThis
fi

tb_misaka_sisters_info=/var/lib/mysql-files/misaka_sisters_info.csv
tb_nicknames=/var/lib/mysql-files/nicknames.csv

if [ -e "$tb_misaka_sisters_info" ]; then
    rm $tb_misaka_sisters_info
fi
if [ -e "$tb_nicknames" ]; then
    rm $tb_nicknames
fi

#
# need to create a file ~/.my.cnf
# and write:
#   [client]
#   password=your_password
#   user=root 
#
# 1. sql file
mysqldump MisakaSisters > $DataDirForThis/db.sql
# 2. csv file
mysql -N < $PROJ_Dir/sql/export.sql
cp $tb_misaka_sisters_info $DataDirForThis
cp $tb_nicknames $DataDirForThis


####################################
#      git tag and release         #
####################################


