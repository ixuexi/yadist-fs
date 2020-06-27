#!/bin/bash

#sudo mount -t proc proc ./simple/proc
#sudo mount --rbind /dev ./test/dev
sudo chroot ./blank/ /usr/bin/env -i HOME=/root TERM=$TERM PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin /bin/bash --login
