#!/bin/sh

# Author:  Jez Tucker
# Date:    12/04/2012
# Version: 1.0

# Generates the partition list in the correct format
# Tested on Linux
# Add the output to the correct section of the nsddevices file in /var/mmfs/etc/

echo "-- Cut here --";

for PART in $(cat /proc/partitions | grep sd | awk '{print $4}' | sort); do 
  echo $PART | egrep -q [0-9] ; 
  if [ $? -eq 0 ]; then 
    echo "echo \"$PART gpt\""; 
  else 
    echo "echo \"$PART generic\""; 
  fi ; 
done

echo "-- Cut here --";

# EOF
