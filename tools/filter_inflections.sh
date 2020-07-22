#!/bin/bash
declare -a inflections=($(cat inflections))
for ((i=0; i<${#inflections[@]}; i++));
do
	m=$(cat unigram.vocab|awk '{print $1}' |grep -w "${inflections[$i]}")
	x=$(cat unigram.vocab |awk '{print $2}')
	echo $x;
	#inflections[$i]="$c ${inflections[$i]}"
	#echo ${inflections[$i]} >> inflections
done
