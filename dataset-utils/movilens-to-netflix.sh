#!/bin/bash
# Transforms MoviLens ratings data into the Netflix dataset format

# Set internal field separator
OIFS=$IFS
IFS=':'

# Clean
rm -r temp
rm -r temp2
rm -r training_set
rm probe.txt
rm qualifying.txt
mkdir temp
mkdir temp2
mkdir training_set

echo "Processing $1..."

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

# Changing userId for users 1-5 to 71568-71572
if [ ${register[0]} -lt 6 ]; then
usr=$((${register[0]} + 71567))
else
usr=${register[0]}
fi

# Simplified rating (rounding up to avoid zeros)
ratlen=$(expr length "${register[4]}")
if [ $ratlen -gt 1 ]; then
rat=$((${register[4]:0:1} + 1))
else
rat=${register[4]}
fi

# MovieLens Timestamp to Netflix Date
dt=$(date --date="@${register[6]}" +%Y-%m-%d)

# Output temporary movie files
echo "$usr,$rat,$dt" >> temp/${register[2]}.tmp

((lines += 1))
fi
done < $1

echo "Processed $lines lines from MovieLens dataset!"

echo "Renaming files to remove gaps..."
# Make temporary movie files numerically sequential
i=1
for f in temp/*.tmp; do
mv $f temp2/$i.tmp
((i += 1))
done

echo "Renamed $(($i - 1)) files to remove gaps..."
echo "Processing temporary files..."
# Restore internal field separator
IFS=$OIFS

probelines=0
# For each temporary movie file
for f in $(ls temp2 | cut -d "." -f 1 | sort -d); do 

# Create sequential movie file and with netflix format (mv_0000001.txt)
fn=`printf mv_%07d.txt $f`
echo "$f:" > training_set/$fn
cat temp2/$f.tmp >> training_set/$fn

# Build probe file and clone qualifying file
# Netflix has 1408395 probes for 100480507 ratings (1,4%)
# Set ~200000 probes for 10000054 Movielens ratings (2%)
# Each time $RANDOM is referenced, a random integer between 0 and 32767 is generated
# Assure that first line won't get selected to probe or else some movies may be empty after scrubbing probe data
# Set internal field separator for building probe and qualifying files
IFS=','
new=true
first=true
while read line; do
if [ $RANDOM -lt 900 ] && ! $first; then
	if $new; then
		echo "$f:" >> probe.txt
		echo "$f:" >> qualifying.txt 
	fi
	declare -a register=($line)
	echo "${register[0]}" >> probe.txt
	echo "${register[0]},${register[2]}" >> qualifying.txt
	new=false
	((probelines += 1))
fi
first=false
done < temp2/$f.tmp

# Restore internal field separator (required for proper
IFS=$OIFS
done

# Remove temporary movie files
rm -r temp
rm -r temp2

printf "Built movie files and probe file with $probelines entries!"