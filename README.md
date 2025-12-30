# io_events
Dead simple Garry's Mod module that tracks file changes and fire hooks on the lua side. Written in Rust for true cross-platform support (Linux, macOS, and Windows).

### Building

1. Clone this repository
2. Make sure you have Rust installed with the correct toolchain:
   ```bash
   rustup install stable
   rustup default stable
   ```
3. Build the module:
   ```bash
   cargo build --release
   ```
4. The compiled binary will be in `target/release/`

### Installation

1. Build the module for your platform (see Building section above)
2. Rename the compiled binary to:
   - Windows: `gmcl_io_events_win64.dll` (client) or `gmsv_io_events_win64.dll` (server)
   - Linux: `gmcl_io_events_linux64.dll` (client) or `gmsv_io_events_linux64.dll` (server)
   - macOS: `gmcl_io_events_osx64.dll` (client) or `gmsv_io_events_osx64.dll` (server)
3. Place the binary in `garrysmod/lua/bin/`

**Note:** Despite the `.dll` extension, Linux and macOS binaries should also use this extension for Garry's Mod compatibility.

### Usage

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