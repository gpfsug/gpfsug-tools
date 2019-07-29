# Sample policy for making files immutable

First we create our fileset, using "noncompliant" mode (in case we make a mistake and need to undo something):

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
an external script. The policy for this can be f.ex. /root/set-immutable.policy containing:

~~~
RULE EXTERNAL LIST 'setimmutable' EXEC '/root/set-immutable.sh' OPTS '7'
RULE 'findImmutable' LIST 'setimmutable' FOR FILESET ('testimmutability') WHERE NOT (MISC_ATTRIBUTES LIKE '%X%')
~~~

When applied using:

~~~
# mmapplypolicy gpfs01 -P /root/set-immutable.policy -I yes
~~~

this find all non-immutable files, and calls the script 
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

# Define expiry date on mmchattr format based on OPTS from policy:
expiry=$(date --date +${3}days  '+%Y-%m-%d')

case $1 in
        TEST )
                exit 0
        ;;
        LIST )
                awk -F ' -- ' '{print $2}' $2 | xargs -d '\n' mmchattr -E $expiry
                awk -F ' -- ' '{print $2}' $2 | xargs -d '\n' mmchattr -i yes
        ;;
esac
~~~

## Testing

First we validate that it finds an acceptable number of files, and that we don't have a bug that changes all files in the filesystem:

~~~
# mmapplypolicy gpfs01 -P /root/set-immutable.policy -I test
<snip>
[I] Summary of Rule Applicability and File Choices:
 Rule#      Hit_Cnt          KB_Hit          Chosen       KB_Chosen          KB_Ill    Rule
     0          100          512000             100          512000               0    RULE 'findImmutable' LIST 'setimmutable' FOR FILESET(.) WHERE(.)

~~~

Yup, we expected 100 files.. so lets do it:

~~~
# mmapplypolicy gpfs01 -P /root/set-immutable.policy -I yes
~~~

After running the above mmapplypolicy, we can see that files has gotten tagget immutable, and can't be deleted:

~~~
# mmlsattr -L /mnt/gpfs01/testimmutability/51/51
file name:            /mnt/gpfs01/testimmutability/51/51
metadata replication: 1 max 2
data replication:     1 max 2
immutable:            yes                           <<<------
appendOnly:           no
indefiniteRetention:  no
expiration Time:      Fri Jun 28 00:00:00 2019      <<<------
flags:
storage pool name:    system
fileset name:         testimmutability
snapshot name:
creation time:        Fri Jun 21 13:45:43 2019
Misc attributes:      ARCHIVE READONLY
Encrypted:            no

# rm -f /mnt/gpfs01/testimmutability/51/51
rm: cannot remove ‘/mnt/gpfs01/testimmutability/51/51’: Read-only file system

# mmchattr -i no /mnt/gpfs01/testimmutability/51/51
/mnt/gpfs01/testimmutability/51/51: Change immutable flag failed: Operation not permitted.
Can not set immutable or appendOnly flag to No and indefiniteRetention flag to Unchanged, Permission denied!

~~~

but since our fileset was only in "noncompliant" iam-mode, we can override the expiry and delete it anyways:

~~~
# mmchattr -E $(date --date yesterday  '+%Y-%m-%d') /mnt/gpfs01/testimmutability/51/51
# rm -f /mnt/gpfs01/testimmutability/51/51

~~~

## Further ideas

* Increase expiry after it runs out?
* Progressive expiry -- Indefinite retention seems like a very long committment. Maybe a better option can be to tag new files for expiry in 7 days, re-tag 7-day expired files to expire in 30 days, re-tag 30-day expired files to expire in 1 year, etc... Then we always have the option of fixing mistakes some time in the future.
