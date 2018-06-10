#!/bin/bash
rm -f inflections inflections.tmp inflection_candidates
for i in $(more unigram.sorted|awk '{print $1}');do ./electron $x $i; x=$i;done &> diffs
# remove all special characters and numbers from inflections.
declare -a inflections=($(egrep '^add|^del' diffs |sed 's/^add //g' |sed 's/^del //' |grep -E '[[:alnum:]]'|egrep -v '\\|\(|\)|\.|,|\]|\]'|sort|uniq))
cat word.vocab |awk '{print $1}' |sed 's/▁//g' | grep -E '[[:alnum:]]' |grep -v '[^a-z\d\s:]' |sort |uniq > vocab_
for ((i=0; i<${#inflections[@]}; i++));
do
	c=$(grep -- "${inflections[$i]}$" vocab_ |wc -l)
	inflections[$i]="$c ${inflections[$i]}"
	echo ${inflections[$i]} >> inflections.tmp
done
sort -n inflections.tmp|grep -v \* |tail -100 |awk '{print $2}' > inflection_candidates
for i in $(cat inflection_candidates);do grep -w "^$i\s" unigram.vocab; done |sort -n -k 2 |grep '[[:lower:]]' > inflections
