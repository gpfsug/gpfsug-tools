# customEviction

Evicts files from all AFM filesets based on a threshold-based list policy.

Here's an example for such list policy:
```
RULE EXTERNAL LIST 'evictList' EXEC ''
RULE 'evictRule' LIST 'evictList'
  FROM POOL 'system'
  THRESHOLD (90,80)
  WEIGHT (CURRENT_TIMESTAMP - ACCESS_TIME)
  WHERE REGEX (misc_attributes,'[P]')
    AND KB_ALLOCATED > 0
```

To use the script you need to modify it and adjust the following variables
(in the 'Configuration'-section of the script):
```
EVICT_FS="yourfilesystem"
EVICT_POOL="yourpool"
LOG="/absolute/path/to/customEviction.log"
```
Specify the name of the filesystem to evict from, the name of the pool to evict
from, as well as a log file (will be created).

Next, copy the customized script to all nodes (`/var/mmfs/etc/customEviction`)
and install it with:
```
mmaddcallback customEviction \
  --command /var/mmfs/etc/customEviction \
  --event lowDiskSpace,noDiskSpace \
  --parms "-e %eventName -f %fsName -p %storagePool"
```

Furthermore, activate the threshold-based list policy with:
```
mmchpolicy <yourfilesystem> actListPol
```
