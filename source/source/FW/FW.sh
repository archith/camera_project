#!/bin/bash
#
function my_exit()
{
	rm -f $FW_name
	echo error exit!!!!!
	exit 1
}

#<------- start script ------->
#include FW map and check parameters
FW_map=FW_map.txt
if [ "$1" != "" ]; then
	FW_map=$1
fi
if [ -f $FW_map ]; then
	source $FW_map
else
	echo ERROR! no FW map: $FW_map
	my_exit
fi

if [ -z $FW_size ] || [ -z $FW_name ] ; then
	echo ERROR! lack of some definitions: FW_size, FW_name
	my_exit
fi

#create a FW block with all FF
./FF $FW_name $FW_size
#echo dd if=/dev/zero  of=FW.bin bs=$FW_size   count=1

#insert images
check_point=0
for (( i=0; i<${#FW_map[@]}; i=i+1 ))
do
	set -- ${FW_map[$i]}
	if (( $check_point > $2 )); then
		echo ERROR! the file \"${FW_map[$(($i-1))]}\" is too big and override \"${FW_map[$(($i))]}\"
		my_exit
	fi
	if [ ! -f $1 ]; then
		echo ERROR! file $1 does not exist
		my_exit
	fi
	if (( $2 == 0 )); then
		dd if=$1 of=$FW_name conv=notrunc > /dev/null 2>&1
	else
		dd if=$1 of=$FW_name bs=$(($2)) seek=1 conv=notrunc > /dev/null 2>&1
	fi
	check_point=$(($2 + $(stat -L -c%s $1)))
done

#insert checksum
for (( i=0; i<${#checksum_map[@]}; i=i+1 ))
do
	set -- ${checksum_map[$i]}
	./checksum $FW_name $1  $(($2-8)) $(($2-8))
	./checksum $FW_name 0x0 $(($2-8)) $(($2-4))
done

echo creating FW completed: $FW_name
