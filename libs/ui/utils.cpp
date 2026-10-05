/* Functions that need cpp support for ui_win.c */

#include <shobjidl.h>
#include <windows.h>

/** 
 * Returns false if an error happened while choosing the folder, else returns true and outBuffer
 * contains the file path
 * **/
extern "C" bool chooseFolder(const wchar_t* title, const wchar_t* defaultFolder, wchar_t* outBuffer) {
	// can be called from any thread: COM must be initialized on it
	HRESULT com = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	IFileDialog* ifd;
	bool success = false;
	HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&ifd));
	if (SUCCEEDED(hr)) {
		if (defaultFolder != nullptr) {
			IShellItem* folder;
			hr = SHCreateItemFromParsingName(defaultFolder, nullptr, IID_PPV_ARGS(&folder));
			if (SUCCEEDED(hr)) {
				ifd->SetDefaultFolder(folder);
				folder->Release();
			}
		}

		ifd->SetOptions(FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
		ifd->SetTitle(title);

		hr = ifd->Show(GetActiveWindow());
		if (SUCCEEDED(hr))
		{
			IShellItem* item;
			hr = ifd->GetResult(&item);
			if (SUCCEEDED(hr))
			{
				wchar_t* wname = nullptr;

				if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &wname)))
				{
					memcpy(outBuffer, wname, (int)(wcslen(wname) + 1) * 2);
					CoTaskMemFree(wname);
					success = true;
				}

				item->Release();
			}
		}

		ifd->Release();
	}

	if (SUCCEEDED(com))
		CoUninitialize();

	return success;
}