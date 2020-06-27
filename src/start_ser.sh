#!/bin/bash

work_dir=/mnt/container
umount ${work_dir}/tmp
umount ${work_dir}/blank
mount -t tmpfs tmpfs ${work_dir}/tmp
source ./env.sh
./store_ser
