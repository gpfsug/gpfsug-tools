# Per user usage summary

Since offline files (HSM/TCT) are not counted in GPFS quota, here's a
sample policy for listing all files and counting file sizes instead of allocated sizes.

```
/*
 * List files
 *
 *  mmapplypolicy gpfs0 -P xxxxxx.policy -I defer -f ./filelist
 *
 */

/* list resident files */
RULE EXTERNAL LIST 'files' EXEC ''

RULE LIST 'files'
     SHOW( VARCHAR(USER_ID) || ' ' ||
          VARCHAR(GROUP_ID) || ' ' ||
          VARCHAR(FILESET_NAME) || ' ' ||
          VARCHAR(FILE_SIZE) || ' ' ||
          VARCHAR(KB_ALLOCATED) )

```

The output file from this can then be piped through awk to give per user summary with:

```
cat file | awk '{a[$4] += $7} END{ for (i in a) print i, a[i]}'
```

