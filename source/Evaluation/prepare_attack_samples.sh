#!/bin/bash

filename=$1
counter=1
na=0
while [ $counter -le 8 ]
do
  origin=$((origin+128004))
  na=$((17-counter))
  echo $origin
  v=$(echo $filename"_"$na)
  vout=$(echo $v".csv")
  echo $v
  cat $v | grep Tim | awk '{print $2","$3","$4","$5","$6","$7","$8","$9","$10","$11","$12","$13","$14","$15","$16","$17","$19","$20","$21","$22","$23","$24","$25","$26","$27","$28","$29","$30","$31","$32","$33","$34","$36",1";}' > $vout
  counter=$((counter+1))
done
