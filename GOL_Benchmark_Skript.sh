OMP_NUM_THREADS=8
timeStamp=$(date +"%y.%m.%d|%H:%M:%S")
log=Benchmarks/times.txt
printf "date,i,program,totaltime,cpu,memory\n" >$log
#parameters:
# 1024 x 1024
inputParameters[0]="350 1024 1024 1 1"
inputParameters[1]="350 512 1024 2 1"
inputParameters[2]="350 1024 512 1 2"
inputParameters[3]="350 256 1024 4 1"
inputParameters[4]="350 512 512 2 2"
inputParameters[5]="350 1024 256 1 4"
inputParameters[6]="350 170 1024 6 1"
inputParameters[7]="350 341 512 3 2"
inputParameters[8]="350 512 341 2 3"
inputParameters[9]="350 1024 170 1 6"
inputParameters[10]="350 128 1024 8 1"
inputParameters[11]="350 256 512 4 2"
inputParameters[12]="350 512 256 2 4"
inputParameters[13]="350 1024 128 1 8"
## 2048 x 2048
inputParameters[14]="110 2048 2048 1 1"
inputParameters[15]="110 1024 2048 2 1"
inputParameters[16]="110 2048 1024 1 2"
inputParameters[17]="110 512 2048 4 1"
inputParameters[18]="110 1024 1024 2 2"
inputParameters[19]="110 2048 512 1 4"
inputParameters[20]="110 341 2048 6 1"
inputParameters[21]="110 683 1024 3 2"
inputParameters[22]="110 1024 683 2 3"
inputParameters[23]="110 2048 341 1 6"
inputParameters[24]="110 256 2048 8 1"
inputParameters[25]="110 512 1024 4 2"
inputParameters[26]="110 1024 512 2 4"
inputParameters[27]="110 2048 256 1 8"
## 4096 x 4096
inputParameters[28]="54 4096 4096 1 1"
inputParameters[29]="54 2048 4096 2 1"
inputParameters[30]="54 4096 2048 1 2"
inputParameters[31]="54 1024 4096 4 1"
inputParameters[32]="54 2048 2048 2 2"
inputParameters[33]="54 4096 1024 1 4"
inputParameters[34]="54 683 4096 6 1"
inputParameters[35]="54 1365 2048 3 2"
inputParameters[36]="54 2048 1365 2 3"
inputParameters[37]="54 4096 683 1 6"
inputParameters[38]="54 512 4096 8 1"
inputParameters[39]="54 1024 2048 4 2"
inputParameters[40]="54 2048 1024 2 4"
inputParameters[41]="54 4096 512 1 8"

#Thread arrangements
#1: 1 1
#2: 2 1
# : 1 2
#4: 4 1
# : 2 2
# : 1 4
#6: 6 1
# : 3 2
# : 2 3
# : 1 6
# gehen nicht!?
#8: 8 1
# : 4 2
# : 2 4
# : 1 8

for j in "${inputParameters[@]}"; do
    for i in {0..4}; do
        printf "%s, %d, " $timeStamp $i >>$log
        /usr/bin/time -o $log -a -f " %C,  %E,  %P, %M" ./gameoflife $j
    done
    #printf "________________________________________________________________________________________________________________________\n" >>$log
done
