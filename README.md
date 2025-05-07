# IRIX xmms output plugin

Should work with xmms 1.2.11.

The source is from SGI Freeware CDs. I fixed a couple of warnings and removed some dead code and incomplete features which did nothing. The original installation instructions are in INSTALL.old. Do not follow them.

## Installation

```
$ CFLAGS='-I/path/to/glib/includes -I/path/to/xmms/includes' make
$ mkdir -p ~/.xmms/Plugins/Output
$ cp libXMMS.so ~/.xmms/Plugins/Output
```

