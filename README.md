# io_events
Dead simple Garry's Mod module that tracks file changes and fire hooks on the lua side. Works on both Linux and Windows.

Note: This is still WIP.

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
