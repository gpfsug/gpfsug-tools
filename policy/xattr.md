# Extended attribute usage

Extened attributes can be added to files with mmchattr:

```
# mmchattr --set-attr user.sendtocloud=yes /gpfs/gpfsmgmt/fil2
# # mmlsattr -L -d  /gpfs/gpfsmgmt/fil2
# <snip>
# user.sendtocloud:     "yes"
```

And found via policy like:

```
# cat policy.pol
RULE EXTERNAL LIST 'files' EXEC ''
RULE LIST 'files' WHERE xattr('user.sendtocloud') LIKE 'yes'

# mmapplypolicy /gpfs/gpfsmgmt/ -P policy.pol -I defer -f /tmp/pol
# cat /tmp/pol.list.files
37390 999490048 0   -- /gpfs/gpfsmgmt/fil2
```
