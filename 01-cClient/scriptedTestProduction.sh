#!/bin/bash
echo $@
for f in $@
do
   echo "sending via: /home/wsadler/workspace/ipp/cClient/cClient 107.23.15.169 11235 $f"
   /home/wsadler/workspace/ipp/cClient/cClient 107.23.15.169 11235 $f
done
