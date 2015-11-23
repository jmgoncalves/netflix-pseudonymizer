#!/bin/bash
# Transforms MoviLens ratings data into the Netflix dataset format

# Set internal field separator
OIFS=$IFS
IFS=':'

echo "Processing $1..."

ratingSum=0
lines=0
# Process movielens file
while read line; do

# Split line in call register array
# 0: UserID 1-71567 (total 71567)
# 2: MovieID 1-65133 (total 10681)
# 4: Rating 1-5 (halves also)
# 6: Timestamp
declare -a register=($line)

# test for valid line
if [ ${#register[@]} -eq 7 ]; then

# Simplified rating (rounding up to avoid zeros)
ratlen=$(expr length "${register[4]}")
if [ $ratlen -gt 1 ]; then
rat=$((${register[4]:0:1} + 1))
else
rat=${register[4]}
fi

((ratingSum += $rat))
((lines += 1))
fi
done < $1

echo "Processed $lines lines from MovieLens dataset and rating sum is $ratingSum!"

# Restore internal field separator
IFS=$OIFS
unset OIFS
