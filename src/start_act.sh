#!/bin/bash

work_dir=/mnt/container
umount ${work_dir}/tmp
umount ${work_dir}/blank
mount -t tmpfs tmpfs ${work_dir}/tmp
source ./env.sh
./store_act -f -o kernel_cache -o allow_other -o fsname=${work_dir}tmp -o modules=subdir -o subdir=${work_dir}/tmp ${work_dir}/blank
