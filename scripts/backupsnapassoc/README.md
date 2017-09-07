# backupsnapassoc

Reads existing snapshot schedule associations defined by Spectrum Scale GUI, and writes them to stdout in a format which can be used to re-apply them later. Optionally deletes all snapshot schedule associations - ensure that you save the output so that you can later re-apply the rules...

This is particularly useful for upgrades, as creation and deletion of snapshots using `mmcrsnapshot` and `mmdelsnapshot` needs to be stopped during the upgrade window.

To simply backup all existing snapshot schedule associations:
```
./backupsnapassoc.sh > snapassocs.sh
```

To backup all snapshot schedule associations and then delete them (run this before the upgrade):
```
./backupsnapassoc.sh --delete > snapassocs.sh
```

To re-apply all snapshot schedule associations (run this after the upgrade):
```
./snapassocs.sh
```
