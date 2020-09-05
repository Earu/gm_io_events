# io_events
Dead simple Garry's Mod module that tracks file changes and fire hooks on the lua side. Works on both Linux and Windows.

### Compiling for linux/macosx
1) Get [premake](https://github.com/premake/premake-core/releases/download/v5.0.0-alpha14/premake-5.0.0-alpha14-linux.tar.gz) add it to your `PATH`
2) Get [garrysmod_common](https://github.com/danielga/garrysmod_common) (with `git clone https://github.com/danielga/garrysmod_common --recursive --branch=x86-64-support-sourcesdk`) and set an env var called `GARRYSMOD_COMMON` to the path of the local repo
3) Run `premake5 gmake --gmcommon=$GARRYSMOD_COMMON` in your local copy of **this** repo
4) Navigate to the makefile directory (`cd /projects/linux/gmake` or `cd /projects/macosx/gmake`)
5) Run `make config=releasewithsymbols_x86_64`

### Usage
Get one the pre-compiled binaries or build it yourself, then put the binary under `garrysmod/lua/bin`.

```lua
require('io_events')

hook.Add("FileChanged", "my_hook", function(path, event_type)
  if path:EndsWith(".lua") and event_type == "DELETED" then
    print("A lua file was removed!")
  end
end)
```

**File Change Event Types:**
- `CREATED` the file was just created
- `CHANGED` the file contents were just modified
- `DELETED` the file was just deleted
- `RENAMED_NEW` the file was just renamed, the path is the new path to the file
- `RENAMED_OLD` the file was just renamed, the path is the old path to the file
- `UNKNOWN` the file went under some kind of change, but we don't know what it was

**Scope:**

This module only targets files among your Garry's Mod directory, other files on your system are not part of the scope of this module.
