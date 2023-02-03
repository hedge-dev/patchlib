# patchlib
A small program to patch linker directives in MSVC object libraries.

Example:
```
patchlib -r "_MSC_VER" library.lib
```

Or if an object is used

```
patchlib -j -r "_MSC_VER" object.obj
```