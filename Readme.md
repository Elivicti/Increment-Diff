# Increment Diff

This util is used for generating increment update patch. It compare file's version by checking its sha1 hash value.

For example, version 1 of `my_dir` looks like this:
```
my_dir
 |- a.txt
 |- b.txt
 |- c.txt
 \- sub_dir
     \- d.txt
```

Then in version 2, `d.txt` is modified, `c.txt` is deleted, `note.txt` is added under `my_dir`:
```
my_dir
 |- a.txt
 |- b.txt
 |- note.txt
 \- sub_dir
     \- d.txt
```

This program is able to make a increment diff:
```
my_dir_ver2_diff
 |- note.txt
 \- sub_dir
     \- d.txt
```
and a script file used for deleting files that are no longer exists in version 2.

How is this useful? I'm not sure. ¯\\_(ツ)_/¯