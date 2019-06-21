# common.include

I like to maintain a few useful macros in a file I call "common.include"
and then include this in all my policies with:


```
  include(/gpfs/gpfsmgmt/etc/common.include)
```

OBS: Don't ut it in an installed policy for the filesystem it's located in! FS will then fail to mount.


```
define( is_premigrated, (MISC_ATTRIBUTES LIKE '%M%' AND (MISC_ATTRIBUTES NOT LIKE '%V%')))
define( is_migrated,    (MISC_ATTRIBUTES LIKE '%V%') )
define( is_resident,    (MISC_ATTRIBUTES NOT LIKE '%M%') )
define( access_age,     (DAYS(CURRENT_TIMESTAMP) - DAYS(ACCESS_TIME)))
define( hours_from_modification,        (CURRENT_TIMESTAMP - MODIFICATION_TIME))
define( mb_allocated,   (INTEGER(KB_ALLOCATED / 1024)))

/* Useful TCT related attributes */
define(EA_FSIZE, XATTR_INTEGER('dmapi.MCEA', 129, 8, 'BIG_ENDIAN'))
define(MTIME_SEC, XATTR_INTEGER('dmapi.MCEA', 9, 8, 'BIG_ENDIAN'))
define(MTIME_NSEC, XATTR_INTEGER('dmapi.MCEA', 17, 8, 'BIG_ENDIAN'))
define(MTIME_XATTR, MTIME_SEC+DECIMAL(MTIME_NSEC/1000)/1000000)
define(MTIME_FILE, DECIMAL(((MODIFICATION_TIME-TIMESTAMP(0)) SECONDS(12,6))))

/* TCT needs timestamp comparison to decide if cloud version is same as local version: */
define( is_premigrated_different, ((XATTR('dmapi.MCEA', 5, 1) == 'C') AND (MTIME_FILE != MTIME_XATTR)))
define( is_premigrated_same, ((XATTR('dmapi.MCEA', 5, 1) == 'C') AND (MTIME_FILE == MTIME_XATTR)))

define(
        exclude_list,
        (
         FALSE
         OR PATH_NAME LIKE '%/.mcstore/%'
         OR PATH_NAME LIKE '%/.mcstore.bak/%'
         OR PATH_NAME LIKE '%/.snapshots/%'
        )
)
define(
        weight_mig_offline,
        (CASE
                /*=== The file is very young, the ranking is very low ===*/
                WHEN access_age <= 1 THEN 0
                /* File is not premigrated, prefer not to select these */
                WHEN NOT(is_premigrated) THEN 1
                /*=== The file is very small, the ranking is low ===*/
                WHEN mb_allocated < 1 THEN access_age
                /*=== The file is resident and large and old enough,
                the ranking is standard ===*/
                ELSE mb_allocated * access_age
        END)
)
define(
        tctfilesets,
        (
          'fileset1',
          'fileset2'
        )
)
```

