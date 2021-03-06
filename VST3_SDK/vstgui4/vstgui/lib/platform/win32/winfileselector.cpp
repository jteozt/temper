//-----------------------------------------------------------------------------
// VST Plug-Ins SDK
// VSTGUI: Graphical User Interface Framework for VST plugins
//
// Version 4.3
//
//-----------------------------------------------------------------------------
// VSTGUI LICENSE
// (c) 2015, Steinberg Media Technologies, All Rights Reserved
//-----------------------------------------------------------------------------
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
// 
//   * Redistributions of source code must retain the above copyright notice, 
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation 
//     and/or other materials provided with the distribution.
//   * Neither the name of the Steinberg Media Technologies nor the names of its
//     contributors may be used to endorse or promote products derived from this 
//     software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
// IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE  OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------

#include "../../cfileselector.h"

#if WINDOWS

#include "win32support.h"
#include "win32frame.h"
#include "../../cstring.h"
#include <shobjidl.h>
#include <shlobj.h>
#include <commdlg.h>
#include <string>

#define IID_PPV_ARG(IType, ppType) IID_##IType, (void**)ppType

extern OSVERSIONINFOEX	gSystemVersion;

namespace VSTGUI {

#define kAllSupportedFileTypesString	L"All supported file types"

//-----------------------------------------------------------------------------
static COMDLG_FILTERSPEC* buildExtensionFilter (std::list<CFileExtension>& extensions, const CFileExtension* defaultExtension, DWORD& numExtensions, DWORD& defaultFileTypeIndex)
{
	if (extensions.empty () == false)
	{
		int32_t i = extensions.size () > 1 ? 1:0;
		COMDLG_FILTERSPEC* filters = new COMDLG_FILTERSPEC[extensions.size ()+1+i];
		int32_t allExtensionCharCount = 0;
		std::list<CFileExtension>::iterator it = extensions.begin ();
		while (it != extensions.end ())
		{
			UTF8StringHelper desc ((*it).getDescription ());
			UTF8StringHelper ext ((*it).getExtension ());
			WCHAR* wDesc = (WCHAR*)std::malloc ((wcslen (desc)+1) * sizeof (WCHAR));
			WCHAR* wSpec = (WCHAR*)std::malloc ((wcslen (ext)+3) * sizeof (WCHAR));
			memcpy (wDesc, desc.getWideString (), (wcslen (desc)+1) * sizeof (WCHAR));
			memcpy (wSpec+2, ext.getWideString (), (wcslen (ext)+1) * sizeof (WCHAR));
			wSpec[0] = 0x002a; // *
			wSpec[1] = 0x002e; // .
			filters[i].pszName = wDesc;
			filters[i].pszSpec = wSpec;
			if (defaultExtension && *defaultExtension == (*it))
				defaultFileTypeIndex = i+1;
			allExtensionCharCount += (int32_t)wcslen (filters[i].pszSpec) + 1;
			it++; i++;
		}
		if (extensions.size () > 1)
		{
			WCHAR* wAllName = (WCHAR*)std::malloc ((wcslen (kAllSupportedFileTypesString)+1) * sizeof (WCHAR));
			wcscpy (wAllName, kAllSupportedFileTypesString);
			WCHAR* wAllSpec = (WCHAR*)std::malloc (allExtensionCharCount * sizeof (WCHAR));
			wAllSpec[0] = 0;
			for (int32_t j = 1; j < i; j++)
			{
				wcscat (wAllSpec, filters[j].pszSpec);
				if (j != i-1)
					wcscat (wAllSpec, L";");
			}
			filters[0].pszName = wAllName;
			filters[0].pszSpec = wAllSpec;
		}
		numExtensions = i;
		filters[i].pszName = 0;
		filters[i].pszSpec = 0;
		return filters;
	}
	return 0;
}

//-----------------------------------------------------------------------------
static void freeExtensionFilter (COMDLG_FILTERSPEC* filters)
{
	if (filters)
	{
		int32_t i = 0;
		while (filters[i].pszName)
		{
			std::free ((void*)filters[i].pszName);
			std::free ((void*)filters[i].pszSpec);
			i++;
		}
		delete [] filters;
	}
}

//-----------------------------------------------------------------------------
class VistaFileSelector : public CNewFileSelector
{
public:
	VistaFileSelector (CFrame* frame, Style style);
	~VistaFileSelector ();

	virtual bool runInternal (CBaseObject* delegate) VSTGUI_OVERRIDE_VMETHOD;
	virtual void cancelInternal () VSTGUI_OVERRIDE_VMETHOD;
	virtual bool runModalInternal () VSTGUI_OVERRIDE_VMETHOD;
protected:
	Style style;
	IFileDialog* fileDialog;
};

//-----------------------------------------------------------------------------
class XPFileSelector : public CNewFileSelector
{
public:
	XPFileSelector (CFrame* frame, Style style);

	virtual bool runInternal (CBaseObject* delegate) VSTGUI_OVERRIDE_VMETHOD;
	virtual void cancelInternal () VSTGUI_OVERRIDE_VMETHOD;
	virtual bool runModalInternal () VSTGUI_OVERRIDE_VMETHOD;
protected:
	Style style;
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CNewFileSelector* CNewFileSelector::create (CFrame* parent, Style style)
{
	if (parent == 0)
	{
		#if DEBUG
		DebugPrint ("Need frame for CNewFileSelector\n");
		#endif
		return 0;
	}
	if (getSystemVersion ().dwMajorVersion >= 6) // Vista
		return new VistaFileSelector (parent, style);
	return new XPFileSelector (parent, style);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#ifndef __GNUC__
typedef HRESULT (STDAPICALLTYPE *SHCreateItemFromParsingNameProc) (__in PCWSTR pszPath, __in_opt IBindCtx *pbc, __in REFIID riid, __deref_out void **ppv);
#else
typedef HRESULT (STDAPICALLTYPE *SHCreateItemFromParsingNameProc) (PCWSTR pszPath, IBindCtx *pbc, REFIID riid, void **ppv);
#endif
SHCreateItemFromParsingNameProc _SHCreateItemFromParsingName = 0;

//-----------------------------------------------------------------------------
VistaFileSelector::VistaFileSelector (CFrame* frame, Style style)
: CNewFileSelector (frame)
, style (style)
, fileDialog (0)
{
	if (_SHCreateItemFromParsingName == 0)
	{
		HINSTANCE shell32Instance = LoadLibraryA ("shell32.dll");
		if (shell32Instance)
		{
			_SHCreateItemFromParsingName = (SHCreateItemFromParsingNameProc)GetProcAddress (shell32Instance, "SHCreateItemFromParsingName");
		}
	}
}

//-----------------------------------------------------------------------------
VistaFileSelector::~VistaFileSelector ()
{
}

//-----------------------------------------------------------------------------
bool VistaFileSelector::runInternal (CBaseObject* delegate)
{
	bool result = runModalInternal ();
	if (result && delegate)
	{
		delegate->notify (this, kSelectEndMessage);
	}
	return result;
}

//-----------------------------------------------------------------------------
void VistaFileSelector::cancelInternal ()
{
	if (fileDialog)
		fileDialog->Close (-1);
}

//-----------------------------------------------------------------------------
bool VistaFileSelector::runModalInternal ()
{
	fileDialog = 0;
	HRESULT hr = -1;
	if (style == kSelectSaveFile)
	{
		hr = CoCreateInstance (CLSID_FileSaveDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IFileDialog, &fileDialog));
		if (defaultSaveName)
		{
			fileDialog->SetFileName (UTF8StringHelper (defaultSaveName));
		}
	}
	else
	{
		hr = CoCreateInstance (CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IFileDialog, &fileDialog));
		if (SUCCEEDED (hr))
		{
			if (style == kSelectDirectory)
			{
				DWORD dwOptions = 0;
				hr = fileDialog->GetOptions (&dwOptions);
				if (SUCCEEDED (hr))
					hr = fileDialog->SetOptions (dwOptions | FOS_PICKFOLDERS);
				if (FAILED (hr))
				{
					fileDialog->Release ();
					fileDialog = 0;
					return false;
				}
			}
			if (allowMultiFileSelection)
			{
				DWORD dwOptions = 0;
				hr = fileDialog->GetOptions (&dwOptions);
				if (SUCCEEDED (hr))
					hr = fileDialog->SetOptions (dwOptions | FOS_ALLOWMULTISELECT);
				if (FAILED (hr))
				{
					fileDialog->Release ();
					fileDialog = 0;
					return false;
				}
			}
		}
	}
	if (FAILED (hr))
	{
		fileDialog = 0;
		return false;
	}

	if (title)
		hr = fileDialog->SetTitle (UTF8StringHelper (title));

	DWORD numExtensions = 0;
	DWORD defaultFileTypeIndex = 0;
	COMDLG_FILTERSPEC* filters = buildExtensionFilter (extensions, defaultExtension, numExtensions, defaultFileTypeIndex);
	if (filters)
	{
		fileDialog->SetFileTypes (numExtensions, filters);
		if (defaultFileTypeIndex)
			fileDialog->SetFileTypeIndex (defaultFileTypeIndex);
	}
	if (initialPath && _SHCreateItemFromParsingName)
	{
		IShellItem* shellItem;
		hr = _SHCreateItemFromParsingName (UTF8StringHelper (initialPath), 0, IID_PPV_ARG (IShellItem, &shellItem));
		if (SUCCEEDED (hr))
		{
			fileDialog->SetFolder (shellItem);
			shellItem->Release ();
		}
	}
	Win32Frame* win32Frame = frame->getPlatformFrame () ? dynamic_cast<Win32Frame*> (frame->getPlatformFrame ()) : 0;
	hr = fileDialog->Show (win32Frame ? win32Frame->getPlatformWindow () : 0);
	if (SUCCEEDED (hr))
	{
		if (allowMultiFileSelection)
		{
			IFileOpenDialog* openFileDialog = 0;
			hr = fileDialog->QueryInterface (IID_PPV_ARG(IFileOpenDialog, &openFileDialog));
			if (SUCCEEDED (hr))
			{
				IShellItemArray* items;
				hr = openFileDialog->GetResults (&items);
				if (SUCCEEDED (hr))
				{
					DWORD count;
					hr = items->GetCount (&count);
					for (DWORD i = 0; i < count; i++)
					{
						IShellItem* item;
						hr = items->GetItemAt (i, &item);
						if (SUCCEEDED (hr))
						{
							LPWSTR filesysPath = 0;
							hr = item->GetDisplayName (SIGDN_FILESYSPATH, &filesysPath);
							if (SUCCEEDED (hr))
							{
								UTF8StringHelper str (filesysPath);
								UTF8StringBuffer resultPath = String::newWithString (str);
								result.push_back (resultPath);
							}
							item->Release ();
						}
					}
					items->Release ();
				}
				openFileDialog->Release ();
			}
		}
		else
		{
			IShellItem *item;
			hr = fileDialog->GetResult (&item);
			if (SUCCEEDED (hr))
			{
				LPWSTR filesysPath = 0;
				hr = item->GetDisplayName (SIGDN_FILESYSPATH, &filesysPath);
				if (SUCCEEDED (hr))
				{
					UTF8StringHelper str (filesysPath);
					UTF8StringBuffer resultPath = String::newWithString (str);
					result.push_back (resultPath);
				}
				item->Release ();
			}
		}
	}
	fileDialog->Release ();
	fileDialog = 0;
	freeExtensionFilter (filters);
	return SUCCEEDED (hr);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
XPFileSelector::XPFileSelector (CFrame* frame, Style style)
: CNewFileSelector (frame)
, style (style)
{
}

//-----------------------------------------------------------------------------
bool XPFileSelector::runInternal (CBaseObject* delegate)
{
	bool result = runModalInternal ();
	if (result && delegate)
	{
		delegate->notify (this, kSelectEndMessage);
	}
	return result;
}

//-----------------------------------------------------------------------------
void XPFileSelector::cancelInternal ()
{
}

//-----------------------------------------------------------------------------
bool XPFileSelector::runModalInternal ()
{
#if DEBUG
	if (allowMultiFileSelection)
	{
		DebugPrint ("CNewFileSelector TODO: multi file selection currently not supported. Please implement this and share it.");
	}
#endif

	if (style == kSelectDirectory)
	{
		UTF8StringHelper titleW (title);
		BROWSEINFO bi = {};
		TCHAR szDisplayName[MAX_PATH]; 
		szDisplayName[0] = 0;  
		bi.hwndOwner = NULL; 
		bi.pidlRoot = NULL; 
		bi.pszDisplayName = szDisplayName; 
		bi.lpszTitle = titleW.getWideString ();
		bi.ulFlags = BIF_RETURNONLYFSDIRS;
		bi.lParam = 0; 
		bi.iImage = 0;  

		LPITEMIDLIST pidl = SHBrowseForFolder (&bi);
		TCHAR szPathName[MAX_PATH]; 
		if (NULL != pidl)
		{
			if (SHGetPathFromIDList(pidl,szPathName))
			{
				char szPathNameC_[MAX_PATH];
				char *szPathNameC= szPathNameC_;
				WideCharToMultiByte (CP_ACP, WC_COMPOSITECHECK|WC_DEFAULTCHAR, szPathName, -1, szPathNameC, MAX_PATH, NULL, NULL); 
				UTF8StringBuffer resultPath = String::newWithString (szPathNameC);
				result.push_back (resultPath);
				return true;
			}
		}
		return false;
	}

	OPENFILENAME ofn = {0};
	ofn.lStructSize  = sizeof (OPENFILENAME);
	ofn.hwndOwner= (HWND)(frame->getPlatformFrame ()->getPlatformRepresentation ());
	ofn.hInstance = GetInstance ();
	std::string filter;
	for (std::list<CFileExtension>::const_iterator it = extensions.begin (); it!=extensions.end (); it++)
	{
		std::string s1= std::string ((it == extensions.begin ()) ? "*." : ";*.");
		std::string s2 = (std::string) (it->getExtension ());
		filter = filter + (s1 + s2);
	}
	UTF8StringHelper filterW (filter.c_str ());
	ofn.lpstrFilter = filterW.getWideString ();
	ofn.nFilterIndex = 1;

	WCHAR filePathBuffer[MAX_PATH];
	*filePathBuffer=0;

	UTF8StringHelper defaultSaveNameW (defaultSaveName);
	if (defaultSaveName)
	{
		wcscpy (filePathBuffer, defaultSaveNameW.getWideString ());
	}
	ofn.lpstrFile = filePathBuffer;
	ofn.lpstrCustomFilter = NULL;
	ofn.nMaxFile = MAX_PATH - 1;
	ofn.lpstrFileTitle = NULL;

	UTF8StringHelper initialPathW (initialPath);
	if (initialPath)
	{
		ofn.lpstrInitialDir = initialPathW.getWideString ();
	}
	else
	{
		ofn.lpstrInitialDir = ofn.lpstrFilter;
	}
	
	UTF8StringHelper titleW (title);
	if (title)
	{
		ofn.lpstrTitle = titleW.getWideString ();
	}
	
	ofn.Flags = OFN_EXPLORER | OFN_ENABLESIZING | OFN_HIDEREADONLY;

	BOOL resultCode = (style == kSelectSaveFile) ? GetSaveFileName (&ofn) : GetOpenFileName (&ofn);
	if (resultCode == 0)
	{
#if DEBUG
		DWORD errcode = CommDlgExtendedError ();
		DebugPrint ("%d\n", errcode);
#endif
		return false;
	}

	UTF8StringHelper str (filePathBuffer);
	UTF8StringBuffer resultPath = String::newWithString (str);
	result.push_back(resultPath);
	return true;
}

} // namespace

#endif // WINDOWS
