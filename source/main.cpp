#include <GarrysMod/FactoryLoader.hpp>
#include <GarrysMod/Lua/Interface.h>
#include <eiface.h>
#include <dbg.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <stdlib.h>

void RefreshDirectory(const char* target_dir)
{
	Msg(target_dir);
	// This is where you might place code to refresh your
	// directory listing, but not the subtree because it
	// would not be necessary.
}

void RefreshTree(const char* target_drive)
{
	Msg(target_drive);
	// This is where you might place code to refresh your
	// directory listing, including the subtree.
}

void WatchDirectory(GarrysMod::Lua::ILuaBase* LUA, const char* target_dir)
{
	DWORD wait_status;
	HANDLE change_handles[2];
	char target_drive[4];
	char target_file[_MAX_FNAME];
	char target_ext[_MAX_EXT];

	_tsplitpath_s(target_dir, target_drive, 4, nullptr, 0, target_file, _MAX_FNAME, target_ext, _MAX_EXT);

	target_drive[2] = '\\';
	target_drive[3] = '\0';

	// Watch the directory for file creation and deletion. 
	change_handles[0] = FindFirstChangeNotification(
		target_dir,                    // directory to watch 
		FALSE,                         // do not watch subtree 
		FILE_NOTIFY_CHANGE_FILE_NAME); // watch file name changes 

	if (change_handles[0] == INVALID_HANDLE_VALUE)
		LUA->ThrowError("Could not get directory handle");

	// Watch the subtree for directory creation and deletion. 
	change_handles[1] = FindFirstChangeNotification(
		target_drive,                  // directory to watch 
		TRUE,                          // watch the subtree 
		FILE_NOTIFY_CHANGE_DIR_NAME);  // watch dir name changes 

	if (change_handles[1] == INVALID_HANDLE_VALUE)
		LUA->ThrowError("Could not get subtree handle");


	// Make a final validation check on our handles.
	if (change_handles[0] == nullptr || change_handles[1] == nullptr)
		LUA->ThrowError("Could not get subtree or directory handles");

	// Change notification is set. Now wait on both notification 
	// handles and refresh accordingly. 
	while (true)
	{
		// Wait for notification.
		wait_status = WaitForMultipleObjects(2, change_handles, FALSE, INFINITE);

		switch (wait_status)
		{
			case WAIT_OBJECT_0:

				// A file was created, renamed, or deleted in the directory.
				// Refresh this directory and restart the notification.
				RefreshDirectory(target_dir);
				if (!FindNextChangeNotification(change_handles[0]))
					LUA->ThrowError("Could not get a new directory handle");

				break;

			case WAIT_OBJECT_0 + 1:

				// A directory was created, renamed, or deleted.
				// Refresh the tree and restart the notification.
				RefreshTree(target_drive);
				if (!FindNextChangeNotification(change_handles[1]))
					LUA->ThrowError("Could not get a new subtree handle");

				break;

			case WAIT_TIMEOUT:

				// A timeout occurred, this would happen if some value other 
				// than INFINITE is used in the Wait call and no changes occur.
				// In a single-threaded environment you might not want an
				// INFINITE wait.
				LUA->ThrowError("WAIT TIMEOUT?");
				break;

			default:
				LUA->ThrowError("Unknown WAIT type?");
				break;
		}
	}
}

std::string GetGamePath(GarrysMod::Lua::ILuaBase *LUA) 
{
	SourceSDK::FactoryLoader engine_loader("engine");
	IVEngineServer* engine_server = engine_loader.GetInterface<IVEngineServer>(INTERFACEVERSION_VENGINESERVER);
	if (engine_server == nullptr)
		LUA->ThrowError("Failed to load required IVEngineServer interface");

	std::string game_dir;
	game_dir.resize(1024);
	engine_server->GetGameDir(&game_dir[0], static_cast<int32_t>(game_dir.size()));
	game_dir.resize(std::strlen(game_dir.c_str()));

	return game_dir;
}

GMOD_MODULE_OPEN()
{
	const char* game_dir = GetGamePath(LUA).c_str();
	WatchDirectory(LUA, game_dir);

	return 0;
}

GMOD_MODULE_CLOSE( )
{
	return 0;
}
