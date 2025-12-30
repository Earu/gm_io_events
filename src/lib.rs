use crossbeam::queue::SegQueue;
use notify::{Event as NotifyEvent, EventKind, RecursiveMode, Watcher};
use std::path::PathBuf;
use std::sync::{Arc, Mutex};

#[macro_use]
extern crate gmod;

// File change event types
#[derive(Debug, Clone)]
enum FileEventType {
    Created,
    Deleted,
    Changed,
    RenamedNew,
    RenamedOld,
    Unknown,
}

impl FileEventType {
    fn as_str(&self) -> &'static str {
        match self {
            FileEventType::Created => "CREATED",
            FileEventType::Deleted => "DELETED",
            FileEventType::Changed => "CHANGED",
            FileEventType::RenamedNew => "RENAMED_NEW",
            FileEventType::RenamedOld => "RENAMED_OLD",
            FileEventType::Unknown => "UNKNOWN",
        }
    }
}

// File change event
#[derive(Debug, Clone)]
struct FileChange {
    path: String,
    event_type: FileEventType,
}

// Global state
static mut FILE_CHANGES: Option<Arc<SegQueue<FileChange>>> = None;
static mut WATCHER: Option<Arc<Mutex<notify::RecommendedWatcher>>> = None;

fn get_file_changes_queue() -> Arc<SegQueue<FileChange>> {
    unsafe {
        let ptr = std::ptr::addr_of_mut!(FILE_CHANGES);
        (*ptr)
            .get_or_insert_with(|| Arc::new(SegQueue::new()))
            .clone()
    }
}

fn convert_notify_event_to_file_event(event: NotifyEvent) -> Vec<FileChange> {
    let mut changes = Vec::new();

    // Handle rename events specially - they provide both old and new paths
    if let EventKind::Modify(notify::event::ModifyKind::Name(rename_mode)) = event.kind {
        use notify::event::RenameMode;
        match rename_mode {
            RenameMode::From => {
                // Old name in a rename operation
                if let Some(path) = event.paths.first() {
                    let path_str = path.to_string_lossy().replace('\\', "/");
                    changes.push(FileChange {
                        path: path_str,
                        event_type: FileEventType::RenamedOld,
                    });
                }
            }
            RenameMode::To => {
                // New name in a rename operation
                if let Some(path) = event.paths.first() {
                    let path_str = path.to_string_lossy().replace('\\', "/");
                    changes.push(FileChange {
                        path: path_str,
                        event_type: FileEventType::RenamedNew,
                    });
                }
            }
            RenameMode::Both => {
                // Both old and new names provided
                if event.paths.len() >= 2 {
                    let old_path = event.paths[0].to_string_lossy().replace('\\', "/");
                    let new_path = event.paths[1].to_string_lossy().replace('\\', "/");

                    changes.push(FileChange {
                        path: old_path,
                        event_type: FileEventType::RenamedOld,
                    });
                    changes.push(FileChange {
                        path: new_path,
                        event_type: FileEventType::RenamedNew,
                    });
                }
            }
            RenameMode::Any | RenameMode::Other => {
                // Unknown rename type, treat as changed
                for path in &event.paths {
                    let path_str = path.to_string_lossy().replace('\\', "/");
                    changes.push(FileChange {
                        path: path_str,
                        event_type: FileEventType::Changed,
                    });
                }
            }
        }
        return changes;
    }

    // Handle other event types
    for path in event.paths.iter() {
        let path_str = path.to_string_lossy().replace('\\', "/");

        let event_type = match event.kind {
            EventKind::Create(_) => FileEventType::Created,
            EventKind::Remove(_) => FileEventType::Deleted,
            EventKind::Modify(_) => FileEventType::Changed,
            EventKind::Access(_) => continue, // Skip access events
            EventKind::Any | EventKind::Other => FileEventType::Unknown,
        };

        changes.push(FileChange {
            path: path_str,
            event_type,
        });
    }

    changes
}

// Ugly but im not feeling like making if checks for each arch/os combination
fn get_game_path() -> Result<String, String> {
    // Get the current working directory
    let mut current = std::env::current_dir()
        .map_err(|e| format!("Failed to get current directory: {e}"))?;

    // The binary might be running from subdirectories like:
    // - GarrysMod/bin/win64/
    // - GarrysMod/bin/
    // - GarrysMod/
    // We need to find the GarrysMod root directory which contains the "garrysmod" folder

    // Try up to 5 parent directories
    for _ in 0..5 {
        // Check if this directory contains "garrysmod" subdirectory
        let garrysmod_path = current.join("garrysmod");
        if garrysmod_path.exists() && garrysmod_path.is_dir() {
            return Ok(current.to_string_lossy().to_string());
        }

        // Go up one directory
        if !current.pop() {
            break;
        }
    }

    Err(format!("Garry's Mod root directory not found"))
}

#[lua_function]
fn spew_file_events(lua: gmod::lua::State) -> i32 {
    unsafe {
        let queue = get_file_changes_queue();

        while let Some(change) = queue.pop() {
            // Call hook.Run("FileChanged", path, event_type)
            lua.get_global(lua_string!("hook"));
            if !lua.is_table(-1) {
                lua.pop();
                continue;
            }

            lua.get_field(-1, lua_string!("Run"));
            if !lua.is_function(-1) {
                lua.pop_n(2);
                continue;
            }

            lua.push_string("FileChanged");
            lua.push_string(&change.path);
            lua.push_string(change.event_type.as_str());

            // Use pcall to catch errors without crashing
            lua.pcall(3, 0, 0);

            lua.pop(); // Pop hook table
        }

        0
    }
}

fn create_dispatcher(lua: gmod::lua::State) {
    unsafe {
        // timer.Create("IOSpewFileEvents", 0.250, 0, spew_file_events)
        lua.get_global(lua_string!("timer"));
        if !lua.is_table(-1) {
            lua.pop();
            return;
        }

        lua.get_field(-1, lua_string!("Create"));
        if !lua.is_function(-1) {
            lua.pop_n(2);
            return;
        }

        lua.push_string("IOSpewFileEvents");
        lua.push_number(0.250);
        lua.push_number(0.0);
        lua.push_function(spew_file_events);

        lua.pcall(4, 0, 0);

        lua.pop(); // Pop timer table
    }
}

fn destroy_dispatcher(lua: gmod::lua::State) {
    unsafe {
        // timer.Remove("IOSpewFileEvents")
        lua.get_global(lua_string!("timer"));
        if !lua.is_table(-1) {
            lua.pop();
            return;
        }

        lua.get_field(-1, lua_string!("Remove"));
        if !lua.is_function(-1) {
            lua.pop_n(2);
            return;
        }

        lua.push_string("IOSpewFileEvents");

        lua.pcall(1, 0, 0);

        lua.pop(); // Pop timer table
    }
}

#[gmod13_open]
fn gmod13_open(lua: gmod::lua::State) -> i32 {
    unsafe {
        // Get the game directory path (current working directory)
        let game_path = match get_game_path() {
            Ok(path) => path,
            Err(err) => {
                lua.error(&format!("Failed to get game path: {err}"))
            }
        };

        let queue = get_file_changes_queue();
        let queue_clone = queue.clone();

        // Create file watcher
        let watcher =
            notify::recommended_watcher(move |res: Result<NotifyEvent, notify::Error>| match res {
                Ok(event) => {
                    let changes = convert_notify_event_to_file_event(event);
                    for change in changes {
                        queue_clone.push(change);
                    }
                }
                Err(e) => {
                    eprintln!("[io_events] File watch error: {e:?}");
                }
            });

        match watcher {
            Ok(mut w) => {
                let path = PathBuf::from(&game_path);

                // Watch the game directory recursively
                if let Err(e) = w.watch(&path, RecursiveMode::Recursive) {
                    lua.error(&format!("Failed to watch directory: {e:?}"));
                }

                // Store the watcher
                let ptr = std::ptr::addr_of_mut!(WATCHER);
                *ptr = Some(Arc::new(Mutex::new(w)));

                // Create the timer dispatcher
                create_dispatcher(lua);
            }
            Err(e) => {
                lua.error(&format!("Failed to create watcher: {e:?}"));
            }
        }

        0
    }
}

#[gmod13_close]
fn gmod13_close(lua: gmod::lua::State) -> i32 {
    unsafe {
        destroy_dispatcher(lua);

        // Drop the watcher
        let ptr = std::ptr::addr_of_mut!(WATCHER);
        *ptr = None;

        // Clear the queue
        let queue = get_file_changes_queue();
        while queue.pop().is_some() {}

        0
    }
}
