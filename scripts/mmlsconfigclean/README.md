# mmlsconfigclean

The randomized output of 'mmlsconfig' makes the output difficult to read. 
```
[common]
minReleaseLevel 4.2.3.0
mmfsLogTimeStampISO8601 no
autoload no
[nm_ces]
prefetchThreads 256
[common]
tscWorkerPool 1024
[ems01,ess01,ess02,ess03,ess04,ess05,ess06,ess07,ess08]
tscWorkerPool 512
[nm_ces]
logWrapAmountPct 5
logWrapThreads 32
maxBufferCleaners 128
maxFileCleaners 128
[common]
ioHistorySize 1k
[ems01,ess01,ems]
ioHistorySize 512
[nm_ces]
prefetchPct 60
[common]
socketMaxListenConnections 500
[ems01,ess01,ems]
socketMaxListenConnections 8192
```

mmlsconfigclean (and mmlsconfigpernodeclass <nodeclass>) do a little postprocessing to bring the output in a format better suited for reading.

```
./mmlsconfigclean |head -n 40

[common]
adminMode central
autoload no
ignorePrefetchLUNCount yes
ioHistorySize 1k
minReleaseLevel 4.2.3.0
mmfsLogTimeStampISO8601 no
socketMaxListenConnections 500
tscWorkerPool 1024
verbsPorts mlx4_0

[ems]
ioHistorySize 512
socketMaxListenConnections 8192

[....]
```