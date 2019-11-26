#include <windows.h>
#include <stdlib.h>
#include <tchar.h>

void RefreshDirectory(LPTSTR);
void RefreshTree(LPTSTR);
void WatchDirectory(LPTSTR);



void WatchDirectory(LPTSTR target_dir)
{
	DWORD wait_status;
	HANDLE change_handles[2];
	TCHAR target_drive[4];
	TCHAR target_file[_MAX_FNAME];
	TCHAR target_ext[_MAX_EXT];

	_tsplitpath_s(target_dir, target_drive, 4, NULL, 0, target_file, _MAX_FNAME, target_ext, _MAX_EXT);

	target_drive[2] = (TCHAR)'\\';
	target_drive[3] = (TCHAR)'\0';

	// Watch the directory for file creation and deletion. 

	change_handles[0] = FindFirstChangeNotification(
		target_dir,                    // directory to watch 
		FALSE,                         // do not watch subtree 
		FILE_NOTIFY_CHANGE_FILE_NAME); // watch file name changes 

	if (change_handles[0] == INVALID_HANDLE_VALUE)
	{
		ExitProcess(GetLastError());
	}

	// Watch the subtree for directory creation and deletion. 

	change_handles[1] = FindFirstChangeNotification(
		target_drive,                  // directory to watch 
		TRUE,                          // watch the subtree 
		FILE_NOTIFY_CHANGE_DIR_NAME);  // watch dir name changes 

	if (change_handles[1] == INVALID_HANDLE_VALUE)
	{
		ExitProcess(GetLastError());
	}


	// Make a final validation check on our handles.

	if ((change_handles[0] == NULL) || (change_handles[1] == NULL))
	{
		ExitProcess(GetLastError());
	}

	// Change notification is set. Now wait on both notification 
	// handles and refresh accordingly. 

	while (TRUE)
	{
		// Wait for notification.

		wait_status = WaitForMultipleObjects(2, change_handles, FALSE, INFINITE);

		switch (wait_status)
		{
			case WAIT_OBJECT_0:

				// A file was created, renamed, or deleted in the directory.
				// Refresh this directory and restart the notification.

				RefreshDirectory(target_dir);
				if (FindNextChangeNotification(change_handles[0]) == FALSE)
				{
					ExitProcess(GetLastError());
				}
				break;

			case WAIT_OBJECT_0 + 1:

				// A directory was created, renamed, or deleted.
				// Refresh the tree and restart the notification.

				RefreshTree(target_drive);
				if (FindNextChangeNotification(change_handles[1]) == FALSE)
				{
					ExitProcess(GetLastError());
				}
				break;

			case WAIT_TIMEOUT:

				// A timeout occurred, this would happen if some value other 
				// than INFINITE is used in the Wait call and no changes occur.
				// In a single-threaded environment you might not want an
				// INFINITE wait.

				break;

			default:
				ExitProcess(GetLastError());
				break;
		}
	}
}

void RefreshDirectory(LPTSTR lpDir)
{
	// This is where you might place code to refresh your
	// directory listing, but not the subtree because it
	// would not be necessary.
}

void RefreshTree(LPTSTR lpDrive)
{
	// This is where you might place code to refresh your
	// directory listing, including the subtree.
}