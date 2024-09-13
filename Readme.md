# Increment Diff

This utility program is used for generating increment update patch. It compare file's version by checking its sha1 hash value.

For example, version 1 of `my_dir` looks like this:

```
my_dir
 |- a.txt
 |- b.txt
 |- c.txt
 \- sub_dir
     \- d.txt
```

Then in version 2, `d.txt` is modified, `c.txt` is deleted, `note.txt` is added:

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

and a script file used for deleting files that no longer exists in version 2.

Then, simply copy paste everything from `my_dir_ver2_diff` to version 1 of `my_dir`, and run script file on `my_dir` (which takes directory path as the first command line argument, and deletes files that no longer exist in version 2). Now, `my_dir` is updated to version 2.

How is this useful? I'm not sure. ¯\\_(ツ)_/¯

## How to build

This program requires C++20, make sure your compiler supports it.

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target IncrementDiff -j 8
```

### Cmake options

`USE_FMT`: set to `1` to let cmake find `fmtlib` and use it for formatting strings. If `USE_FMT` is not set, or is set to `1` but `fmtlib` is not found, `std::format` is used to format strings.

## Dependencies

+ [`CryptoPP`](https://github.com/weidai11/cryptopp) and [`cryptopp-cmake`](https://github.com/abdes/cryptopp-cmake):

	Used to compute file's hash value.

+ [`CLI11`](https://github.com/CLIUtils/CLI11):

	Used to create command line interface.

+ [`fmtlib`](https://github.com/fmtlib/fmt) (Optional)

	Optional library for formatting strings, see [Cmake options](#Cmake-options) for more details.