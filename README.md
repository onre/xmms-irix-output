# IRIX xmms output plugin

Source is from SGI Freeware CDs. I fixed a couple of warnings and removed some dead code and incomplete features which did nothing. The original installation instructions are in INSTALL.old.

## Installation

```
$ CFLAGS='-I/path/to/glib/includes -I/path/to/xmms/includes' make
$ mkdir -p ~/.xmms/Plugins/Output
$ cp libXMMS.so ~/.xmms/Plugins/Output
```

