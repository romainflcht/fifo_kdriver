#!/bin/sh

# Script that create and remove every character device interfaces used by the 
# kernel module in /dev/. 

if [ "$USER" != "root" ] ; then
   echo "~[ERR] This script needs root privileges to be run." >&2
   exit 1
fi

cdev_name="fifo"
cdev_count=2
username=romain
major=`grep $cdev_name /proc/devices |cut -d' ' -f1`

# Check if the -u flag has been used. If yes, remove all interfaces. 
del=0
if [ "$1" != "" ]; then
   if [ "$1" = '-u' ] ; then
      del=1
   else
      echo "Usage : $0 [-u]"
      echo "    -u uninstall : removes previously created interfaces (/dev/alpha*)"
      exit 1
   fi
fi

# Check if the module has been loaded into the kernel. 
if [ "$major" = "" ]; then
   echo "~[ERR] Module not loaded in the kernel, use insmod to do it." >&2
   exit 2
fi
echo "~[INFO] MAJOR number is: $major."
i=0
while [ $i -lt $cdev_count ]
do
   dev=/dev/$cdev_name$i
   if [ $del -eq 1 ]; then
      [ -e $dev ] && rm -f $dev
   else
      if [ ! -e $dev ] ; then
            mknod $dev c $major $i
            sudo chown -R $username: $dev
      else
            echo "~[ERR] $dev already exist, abort."
      fi
   fi
   i=`expr $i + 1`
done

exit 0
