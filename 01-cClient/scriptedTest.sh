#!/bin/bash
echo $@
for f in $@
do
   echo "sending via: /home/wsadler/workspace/ipp/cClient/cClient 192.168.1.11 11235 $f"
   /home/wsadler/workspace/ipp/cClient/cClient 192.168.1.11 11235 $f
done
