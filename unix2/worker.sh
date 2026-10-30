#!/bin/sh
 
set -u

SHARED_DIR="/shared"
LOCK_FILE="$SHARED_DIR/.lock"
CONTAINER_ID="$(hostname)"
counter=1
mkdir -p "$SHARED_DIR"
exec 200>>"$LOCK_FILE"
while :; do
	(
		flock -x 200
		n=1
		while :; do
			name=$(printf "%03d" "$n")
			if [ ! -e "$SHARED_DIR/$name" ]; then
				echo "$CONTAINER_ID $counter" > "$SHARED_DIR/$name"
                		echo "Created: $name ($CONTAINER_ID #$counter)"
                		echo "$name" > "/tmp/current_name_$$"
				break
			fi
			n=$((n+1))
		done
	)
	sleep 1
	name=$(cat "/tmp/current_name_$$")
	(
		flock -x 200
		if [ -e "$SHARED_DIR/$name" ]; then
			rm -f "$SHARED_DIR/$name"
			echo "Deleted: $name"
		fi
	)
	rm -f "/tmp/current_name_$$"
	counter=$((counter + 1))
	sleep 1
done
