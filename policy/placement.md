# Placement rules:

Only the following attributes are valid for placement rules:

    * FILESET_NAME
    * GROUP_ID
    * NAME
    * USER_ID

## Sample rules:

```
  # cat policy-db.rule
  RULE 'important-rule' SET POOL 'system' WHERE LOWER(NAME) LIKE '%.xls'
  RULE 'homes' SET POOL 'silver' FOR FILESET ('homefileset')
  RULE 'db' SET POOL 'system' FOR FILESET ('dbfileset')
  RULE 'default' SET POOL 'data'
```

## Randomize placement between two pools:

```
  RULE 'GL2S' SET POOL 'data1' LIMIT(90) WHERE INTEGER(RAND()*30)<20
  RULE 'GL2' SET POOL 'data2'
```

## Hack for preventing storing a given filetype:

  Create a descOnly NSD as only disk in a dummy-pool, and use rule like:

```
  RULE 'do-not-place-mp3' SET POOL 'dummy' WHERE LOWER (NAME) LIKE '%.mp3%'
```


## Overflow placement:

```
  /* By default place files in fast pool, but fail over to slow pool if
     usage is above 95% */
  RULE 'new' SET POOL 'fast' LIMIT(95)
  RULE 'spillover' SET POOL 'slow'
```

## Protect some files with replication

At TCT config for university of xyz, we too often experienced corrupted
TCT database when we had down disks or crashes. Therefore we implemented
replication protection for the TCT database files. This was done by:

* creating a separate group account with gid = 2222
* chowning the .mcstore folder to "root:2222"
* adding sticky group (chmod g+s .mcstore) i
* and the following policy:

```
  RULE 'repMcstore' SET POOL 'system'
  REPLICATE(2) WHERE GROUP_ID = 2222
```

