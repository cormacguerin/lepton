#!/bin/bash
rm -f syns
declare -a inflections=($(cat inflections|awk '{print $1}'))
declare -a unigrams=($(cat unigram.vocab|awk '{print $1}' |grep -v '[^a-z\d\s:]'|sort))
declare -a vocab=($(cat vocab_ |grep '[a-z\d\s:]'|sort))

for i in ${inflections[@]};
do
	echo " - " $i
	syn=$i
	lastsyn=""
	for j in ${vocab[@]};
	do
		x=$(echo $i |grep $j$)
		if [[ -n $x ]]
		then
			stem=$(echo $i |sed s/$j$//g)
			if echo $lastsyn |grep -w $stem
			then
				echo "same stem"
			else
				echo $syn >> syns
			fi
			y=$(awk '{print $1}' word.vocab|grep -w ^▁$stem$)
			if [[ -n $y ]]
			then
				echo "   - " $stem "("$j")"
				syn=$(echo $syn $stem)
			fi
		fi
	done
	lastsyn=$syn
done
