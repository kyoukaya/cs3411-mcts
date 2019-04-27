#!/bin/bash

# Play agent against specified program
# Example:
# ./playc.sh lookt 12345

if [ "$#" -ne 1 ]; then
  echo "Usage: $0 <port>" >&2
  exit 1
fi

./servt -p $1 & sleep 0.1
./lookt -p $1 -d 3
# ./randt -p $1 