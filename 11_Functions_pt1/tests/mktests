#!/bin/sh
# Make the output files for each test

if [ ! -f ../comp1 ]
then echo "Need to build ../comp1 first!"; exit 1
fi

for i in input*
do if [ ! -f "out.$i" ]
   then
     ../comp1 $i
     cc -o out out.s
     ./out > out.$i
     rm -f out out.s
   fi
done
