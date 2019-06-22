# Sample policy for making files immutable

First we create our fileset:

~~~
# mmcrfileset gpfs01 testimmutability --inode-space new
Fileset testimmutability created with id 2 root inode 131075.
# mmlinkfileset gpfs01 testimmutability -J /mnt/gpfs01/testimmutability
Fileset testimmutability linked at /mnt/gpfs01/testimmutability
# mmchfileset gpfs01 testimmutability --iam-mode noncompliant
Fileset testimmutability changed.
~~~

and some files:

~~~
# cd /mnt/gpfs01/testimmutability
# for i in $(seq 1 100); do mkdir $i ; dd if=/dev/zero of=$i/$i bs=1M count=5 ; done
~~~

Unfortunately we can't manipulate the immutability directly in ILM policy, so
we have to use the policy to generate a list of files -- and then give this to
an external script. The policy for this can be f.ex. /root/set-immutable.policy:

~~~
RULE EXTERNAL LIST 'setimmutable' EXEC '/root/set-immutable.sh' OPTS '7'
RULE 'findImmutable' LIST 'setimmutable' FOR FILESET ('testimmutability') WHERE NOT (MISC_ATTRIBUTES LIKE '%X%')
~~~

When applied using:

~~~
# mmapplypolicy gpfs01 -P /root/set-immutable.policy -I yes
~~~

This find all non-immutable files, and calls the script 
"/root/set-immutable.sh" with the arguments $1=LIST, $2=FileList, $3=OPTS. 
In the above policy our OPTS=7, which we use to tell how many days the 
files should be immutable.  So we create a simple /root/set-immutable.sh 
containing:

~~~
#! /bin/sh -
#
# Arguments to this script:
#
# $@ = LIST /mnt/gpfs01/.mmSharedTmpDir/mmPolicy.ix.19751.02F6A4F0.1 7
#
#

# Make sure $3 is an integer, otherwise exit.
if ! test "$3" -eq "$3" 
then
	echo OPTS should be an integer number of days for expiry
	echo OPTS=$3 exiting..
	exit 1
fi

expiry=$(date --date +${3}days  '+%Y-%m-%d')

case $1 in
        TEST )
                exit 0
        ;;
        LIST )
                awk '{print 5}' $2 | xargs mmchattr -E $expiry
                awk '{print 5}' $2 | xargs mmchattr -i yes
        ;;
esac
~~~
