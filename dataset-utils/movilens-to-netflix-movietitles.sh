#!/bin/bash
# Transforms MoviLens movie title data into the Netflix movie title format
#1::Toy Story (1995)::Adventure|Animation|Children|Comedy|Fantasy
#1,1995,Toy Story

# Set internal field separator
OIFS=$IFS
IFS=':'

# Clean
rm movie_titles.txt

echo "Processing $1 to build movie titles..."

lines=1
# Process movielens file
while read line; do

# Split line in call register array
# 0: MovieID 1-65133 (total 10681) - is dropped to make sequential ids
# 2: Title and Year
declare -a register=($line)

# test for valid line
if [ ${#register[@]} -eq 5 ]; then

# getting Title and Year
length=$(expr length "${register[2]}")
title=${register[2]:0:$length - 7}
year=${register[2]:$length - 5:4}

# Output temporary movie files
echo "$lines,$year,$title" >> movie_titles.txt

((lines += 1))
fi
done < $1

echo "Processed $(($lines - 1)) movie titles from MovieLens!"

# Restore internal field separator
IFS=$OIFS
unset OIFS