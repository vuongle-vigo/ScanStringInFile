//////////////////////////////////////////////////////////////////////////////////////////////
// Description: Find string in file
// Author: Vigo0x1
// Copyright. All rights reserved
// Additional information: N/A
// Run in windows 64 bits
//////////////////////////////////////////////////////////////////////////////////////////////

#include <Windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <commctrl.h> 
#include <shlobj.h>
#include <Shlwapi.h>

  
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "Shlwapi.lib")

using namespace std;


#define MAX_LOADSTRING 100
#define IDC_SEARCH_BUTTON 101
#define IDC_STOP_BUTTON 102
#define IDC_FOLDER_PATH 103
#define IDC_CHOOSE_FOLDER_BUTTON 104
#define IDC_SEARCH_TEXT 105
#define IDC_RESULT_LISTVIEW 106

//Global variable
HWND g_hWnd;
HINSTANCE g_hInst;
WCHAR g_szTitle[MAX_LOADSTRING] = L"Find string Application";
WCHAR g_szWindowsClass[MAX_LOADSTRING] = L"Windows Class";
HWND g_hWndBtnSearch, g_hWndBtnStop, g_hWndBtnChooseFolder, g_hWndEdtPathBox, g_hWndEdtStringBox, g_hWndListView;
WCHAR g_byPath[MAX_PATH];
WCHAR g_byStringFind[MAX_LOADSTRING];
HANDLE g_hThreadScanFolder;
HANDLE g_hThreadFindString;
bool g_bStopFlag = FALSE;


//declare function
ATOM				MyRegisterClass(HINSTANCE);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
void SelectFolder();
void StartSearch();
DWORD WINAPI ThreadProcScanFolder(LPVOID);
void FindFileInFolder(LPWSTR);
DWORD WINAPI ThreadProcFindStringInFile(LPVOID);
void FindStringInFile(LPWSTR);
void KMPSearch(const char*, DWORD, const char*, LPWSTR);
void StopButtonHandle();

// Entry point of the application
int WINAPI wWinMain(_In_ HINSTANCE hInst, _In_opt_ HINSTANCE hPreInst, _In_ LPWSTR pszArgs, _In_ int nCmdshow) {
	// Register the window class
	MyRegisterClass(hInst);
	// Initialize the application instance
	if (!InitInstance(hInst, nCmdshow)) {
		return FALSE; // Return FALSE if initialization fails
	}

	MSG msg;
	// Enter the main message loop
	while (GetMessage(&msg, NULL, NULL, NULL)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam; // Return the exit code from the message loop
}

// Register the window class
ATOM MyRegisterClass(HINSTANCE hInstance) {
	WNDCLASSEXW wndClassex;
	// Set the size of the structure
	wndClassex.cbSize = sizeof(WNDCLASSEXW);
	// Set the window class styles
	wndClassex.style = CS_HREDRAW | CS_VREDRAW;
	// Set the window procedure function pointer
	wndClassex.lpfnWndProc = WndProc;
	// Set the number of extra class bytes
	wndClassex.cbClsExtra = 0;
	// Set the number of extra window bytes
	wndClassex.cbWndExtra = 0;
	// Set the application instance handle
	wndClassex.hInstance = hInstance;
	// Set the window icon handle
	wndClassex.hIcon = NULL;
	// Set the default cursor handle
	wndClassex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	// Set the window background brush
	wndClassex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	// Set the menu name (not used in this case)
	wndClassex.lpszMenuName = NULL;
	// Set the window class name
	wndClassex.lpszClassName = g_szWindowsClass;
	// Set the small icon handle for the window
	wndClassex.hIconSm = NULL;
	// Register the window class
	return RegisterClassExW(&wndClassex);
}

// Initialize the application instance
BOOL InitInstance(HINSTANCE hInstance, int nCmdshow) {
	// Store the application instance handle
	g_hInst = hInstance;
	// Create the main window
	HWND hWnd = CreateWindowW(g_szWindowsClass, g_szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, 800, 800, NULL, NULL, hInstance, NULL);
	if (!hWnd) {
		return FALSE; // Return FALSE if window creation failed
	}
	// Show and update the main window
	ShowWindow(hWnd, nCmdshow);
	UpdateWindow(hWnd);

	return TRUE; // Return TRUE to indicate successful initialization
}

//-------------------------------------------------------------------------------------------------------------
// Name: LRESULT CALLBACK::WndProc
// Description: Window procedure function
// Parameters: hWnd: A handle to the window procedure's window.
//			message: The message code.
//			wParam : Additional message - specific information.Its meaning depends on the message value.
//			lParam : Additional message - specific information.Its meaning depends on the message value.
// Return: N/A
//-------------------------------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	g_hWnd = hWnd;
	switch (message)
	{
	case WM_CREATE:
	{
		// Create the path edit box
		g_hWndEdtPathBox = CreateWindowW(L"EDIT", NULL, WS_VISIBLE | WS_CHILD | WS_BORDER,
			40, 30, 300, 25, hWnd, (HMENU)IDC_FOLDER_PATH, NULL, NULL);

		// Create the Choose Folder button
		g_hWndBtnChooseFolder = CreateWindowW(L"BUTTON", L"Choose Folder", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			360, 30, 150, 25, hWnd, (HMENU)IDC_CHOOSE_FOLDER_BUTTON, NULL, NULL);

		// Create the Edit String Box
		g_hWndEdtStringBox = CreateWindowW(L"EDIT", NULL, WS_VISIBLE | WS_CHILD | WS_BORDER,
			40, 70, 340, 25, hWnd, (HMENU)IDC_SEARCH_TEXT, NULL, NULL);

		// Create the Search
		g_hWndBtnSearch = CreateWindowW(L"BUTTON", L"Search", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			40, 110, 75, 25, hWnd, (HMENU)IDC_SEARCH_BUTTON, NULL, NULL);
		//Create Stop button
		g_hWndBtnStop = CreateWindowW(L"BUTTON", L"STOP", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			130, 110, 75, 25, hWnd, (HMENU)IDC_STOP_BUTTON, NULL, NULL);
		
		// Initializing ListView
		INITCOMMONCONTROLSEX icex;
		icex.dwICC = ICC_LISTVIEW_CLASSES;
		InitCommonControlsEx(&icex);

		// Create ListView
		g_hWndListView = CreateWindowEx(0, WC_LISTVIEW, NULL, WS_CHILD | WS_VISIBLE | LVS_REPORT,
			10, 170, 770, 550, hWnd, (HMENU)IDC_RESULT_LISTVIEW, NULL, NULL);
		// Add columns to the ListView
		LVCOLUMN lvColumn;
		lvColumn.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;

		lvColumn.fmt = LVCFMT_LEFT;
		lvColumn.cx = 50;
		lvColumn.pszText = (LPWSTR)L"STT";
		ListView_InsertColumn(g_hWndListView, 0, &lvColumn);

		lvColumn.cx = 250;
		lvColumn.pszText = (LPWSTR)L"Tên";
		ListView_InsertColumn(g_hWndListView, 1, &lvColumn);

		lvColumn.cx = 410;
		lvColumn.pszText = (LPWSTR)L"Đường dẫn";
		ListView_InsertColumn(g_hWndListView, 2, &lvColumn);

		lvColumn.cx = 60;
		lvColumn.pszText = (LPWSTR)L"Vị trí";
		ListView_InsertColumn(g_hWndListView, 3, &lvColumn);

		break;
	}
	case WM_COMMAND:
	{	
		int wmId = LOWORD(wParam);
		int wmEvent = HIWORD(wParam);
		//if modify edit box folder path => reset
		if (wmEvent == EN_CHANGE && (wmId == IDC_FOLDER_PATH || wmId == IDC_SEARCH_TEXT)) {
			if (g_hThreadScanFolder != NULL) {
				CloseHandle(g_hThreadFindString);
				g_hThreadFindString = NULL;
				CloseHandle(g_hThreadScanFolder);
				g_hThreadScanFolder = NULL;
			}
			ListView_DeleteAllItems(g_hWndListView);
			SetWindowText(g_hWndBtnSearch, L"Search");
			break;
		}
		switch (LOWORD(wParam)) {
		case IDC_CHOOSE_FOLDER_BUTTON:
			SelectFolder();
			break;
		case IDC_SEARCH_BUTTON:
		{
			// Check if the search thread handle is not NULL
			if (g_hThreadFindString != NULL) {
				// Resume the execution of the search thread
				ResumeThread(g_hThreadFindString);
				EnableWindow(g_hWndBtnChooseFolder, FALSE);
				EnableWindow(g_hWndBtnSearch, FALSE);
				EnableWindow(g_hWndEdtPathBox, FALSE);
				EnableWindow(g_hWndEdtStringBox, FALSE);
			}
			else {
				// Start the search process
				StartSearch();
			}
			g_bStopFlag = FALSE;
			break;
		}
		case IDC_STOP_BUTTON:
			StopButtonHandle();
			break;
		}
		break;
	}
	case WM_DESTROY:
		// Quit the application
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

//-------------------------------------------------------------------------------------------------------------
// Name: void::SelectFolder
// Description: Open windows to choose folder and set into editbox
// Parameters: N/A
// Return: N/A
//-------------------------------------------------------------------------------------------------------------
void SelectFolder() {
	// Buffer to hold the selected folder path
	wchar_t byFolderPath[MAX_PATH];
	// Title of the folder selection dialog
	const wchar_t* Title = L"Select Folder\0";

	// Initialize the BROWSEINFO structure
	BROWSEINFOW browseinfo = { 0 };
	browseinfo.hwndOwner = NULL;
	// Set the root folder to NULL for the dialog to start from the default location
	browseinfo.pidlRoot = NULL;
	// Pointer to the buffer that receives the display name of the selected folder
	browseinfo.pszDisplayName = byFolderPath;
	// Set the title of the folder selection dialog
	browseinfo.lpszTitle = Title;
	// Specify the flags for the dialog behavior
	browseinfo.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
	// Set the callback function pointer to NULL
	browseinfo.lpfn = NULL;
	// Pass the buffer as the lParam for the callback function
	browseinfo.lParam = (LPARAM)byFolderPath;

	// Show the folder selection dialog and get the selected folder path
	LPITEMIDLIST pidl = SHBrowseForFolderW(&browseinfo);
	if (pidl != 0) {
		// Get the selected folder path
		SHGetPathFromIDListW(pidl, byFolderPath);
		// Set the converted wide character string as the text of the edit box
		SetWindowTextW(g_hWndEdtPathBox, byFolderPath);
	}
}

//Start search function
void StartSearch() {
	// Get the text from the string input edit box
	if (!GetWindowText(g_hWndEdtStringBox, (LPWSTR)g_byStringFind, 100)) {
		// Display an error message if the input string is empty
		MessageBox(g_hWnd, L"Please enter the string you want to find in the file", L"String invalid", MB_OK);
		return;
	}
	// Get the text from the path input edit box
	GetWindowText(g_hWndEdtPathBox, g_byPath, MAX_PATH);
	// Create a new thread to scan the specified folder
	g_hThreadScanFolder = CreateThread(NULL, 0, ThreadProcScanFolder, g_byPath, 0, NULL);
}

DWORD WINAPI ThreadProcScanFolder(LPVOID lpParameter) {
	// Cast the thread parameter to a wide character string pointer
	wchar_t* byFilepath = (wchar_t*)lpParameter;
	//Disable button and edit box
	EnableWindow(g_hWndBtnSearch, FALSE);
	EnableWindow(g_hWndBtnChooseFolder, FALSE);
	EnableWindow(g_hWndEdtPathBox, FALSE);
	EnableWindow(g_hWndEdtStringBox, FALSE);
	ListView_DeleteAllItems(g_hWndListView);
	// Call the FindFileInFolder function to scan the specified folder
	FindFileInFolder(byFilepath);
	//Enable button and edit box
	EnableWindow(g_hWndBtnSearch, TRUE);
	EnableWindow(g_hWndBtnChooseFolder, TRUE);
	EnableWindow(g_hWndEdtPathBox, TRUE);
	EnableWindow(g_hWndEdtStringBox, TRUE);
	SetWindowText(g_hWndBtnSearch, L"Search");
	CloseHandle(g_hThreadScanFolder);
	g_hThreadScanFolder = NULL;
	// Return a value indicating the thread completion status
	return 1;
}

//-------------------------------------------------------------------------------------------------------------
// Name: void::FindFileInFolder
// Description: find file in folder and scan file
// Parameters: LPWSTR lpPath: path folder
// Return: N/A
//-------------------------------------------------------------------------------------------------------------
void FindFileInFolder(LPWSTR lpPath) {
	// Structure to hold file information
	WIN32_FIND_DATAW wfdDataFind;
	// Get the attributes of the specified path
	DWORD dwAttributes = GetFileAttributesW(lpPath);
	// Check if the path is a valid directory
	if (dwAttributes != INVALID_FILE_ATTRIBUTES && (dwAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
		// Add a backslash to the end of the path
		PathAddBackslashW(lpPath);
		// Temporary buffer for path manipulation
		wchar_t wcTmpPath[MAX_PATH] = L"";
		wcscpy_s(wcTmpPath, lpPath); // Copy the path to the temporary buffer
		// Combine the path with "*"
		PathCombineW(wcTmpPath, wcTmpPath, L"*");
		HANDLE hFind = FindFirstFileW(wcTmpPath, &wfdDataFind); // Find the first file in the specified directory
		// Check if the handle is valid
		if (hFind == INVALID_HANDLE_VALUE)
			return;
		do {	
			wchar_t wcharResult[MAX_PATH] = { 0 }; // Buffer to hold the full path of the file
			wcscpy_s(wcharResult, lpPath); // Copy the base path to the result buffer
			wcscat_s(wcharResult, wfdDataFind.cFileName); // Concatenate the file name to the result buffer
			//Check path is file path
			if (!(wfdDataFind.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				g_hThreadFindString = CreateThread(NULL, 0, ThreadProcFindStringInFile, wcharResult, 0, NULL);
				if (g_hThreadFindString != NULL) {
					WaitForSingleObject(g_hThreadFindString, INFINITE);
					CloseHandle(g_hThreadFindString);
					g_hThreadFindString = NULL;
				}
			}
			else {
				// Found a directory, but ignore '.' and '..'
				if (wcscmp(wfdDataFind.cFileName, L".") != 0
					&& wcscmp(wfdDataFind.cFileName, L"..") != 0) {
					FindFileInFolder(wcharResult); // recurse into the new subdir
				}
			}
			// Find the next file in the directory
		} while (FindNextFileW(hFind, &wfdDataFind));
		FindClose(hFind); // Close the handle to release resources
	}
	else {
		MessageBox(g_hWnd, L"Invalid folder path", L"Error", MB_OK);
	}
	return;
}


// Function to be executed by a thread, finding a string in a file
DWORD WINAPI ThreadProcFindStringInFile(LPVOID lpParameter) {
	// Cast the thread parameter to a wide character string pointer
	wchar_t* byFilepath = (wchar_t*)lpParameter;
	// Call the FindStringInFile function to scan the specified file
	FindStringInFile(byFilepath);
	// Return a value indicating the thread completion status
	return 1;
}

//-------------------------------------------------------------------------------------------------------------
// Name: void::FindStringInFile
// Description: Find string in file and print to listview
// Parameters: LPWSTR lpPath: file paht
// Return: N/A
//-------------------------------------------------------------------------------------------------------------
void FindStringInFile(LPWSTR lpPath) {
	// Open the file specified by the path
	HANDLE hFile = CreateFile(lpPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		// Create a file mapping object for the opened file
		HANDLE hMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
		if (hMapping != NULL)
		{
			// Map the file into memory for efficient access
			const char* data = static_cast<const char*>(MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0));
			if (data != NULL)
			{
				// Get the text to search from a text box (assuming g_hWndEdtStringBox is a valid handle)
				wchar_t byStringFind[100];
				GetWindowText(g_hWndEdtStringBox, (LPWSTR)byStringFind, 100);
				// Convert the wide character string (UTF-16) to multi-byte string (UTF-8)
				int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, (LPWSTR)byStringFind, -1, NULL, 0, NULL, NULL);
				char* byCharString = new char[sizeNeeded];
				WideCharToMultiByte(CP_UTF8, 0, (LPWSTR)byStringFind, -1, byCharString, sizeNeeded, NULL, NULL);
				DWORD dwDataLength = GetFileSize(hFile, NULL);
				// Perform the search operation (assuming KMPSearch is defined and implemented)
				KMPSearch(data, dwDataLength, byCharString, lpPath);
				delete[] byCharString;
				// Unmap the file from memory
				UnmapViewOfFile(data);
			}
			// Close the file mapping object
			CloseHandle(hMapping);
		}
		// Close the file handle
		CloseHandle(hFile);
	}
}

//Create LPSArray for KMP 
vector<int> createLPSArray(const char* pattern, int patternLength) {
	vector<int> lps(patternLength, 0);
	int length = 0;
	int i = 1;

	while (i < patternLength) {
		if (pattern[i] == pattern[length]) {
			length++;
			lps[i] = length;
			i++;
		}
		else {
			if (length != 0) {
				length = lps[length - 1];
			}
			else {
				lps[i] = 0;
				i++;
			}
		}
	}

	return lps;
}

//-------------------------------------------------------------------------------------------------------------
// Name: void::KMPSearch
// Description: KMP algorithm
// Parameters: const char* text: text to search
//			const char* pattern: text search
// Return: N/A
//-------------------------------------------------------------------------------------------------------------
void KMPSearch(const char* text, DWORD dataLenght,const char* pattern, LPWSTR lpPath) {
	char* byDataCopy = new char[dataLenght + 1];
	memcpy(byDataCopy, text, dataLenght);
	byDataCopy[dataLenght] = '\0';
	int textLength = (int)strlen(byDataCopy);
	delete[] byDataCopy;
	int patternLength = (int)strlen(pattern);
	vector<int> lps = createLPSArray(pattern, patternLength);
	int i = 0;
	int j = 0;
	while (i < textLength) {
		if (pattern[j] == text[i]) {
			i++;
			j++;
		}

		if (j == patternLength) {
//result	set to listview		
			LVITEM lvItem;
			lvItem.mask = LVIF_TEXT;
			lvItem.iItem = ListView_GetItemCount(g_hWndListView);
			// Column 1
			lvItem.iSubItem = 0;
			std::wstring sStringIndex = std::to_wstring(ListView_GetItemCount(g_hWndListView));
			LPWSTR pszIndex = new wchar_t[sStringIndex.size() + 1];
			wcscpy_s(pszIndex, sStringIndex.size() + 1, sStringIndex.c_str());
			lvItem.pszText = pszIndex;
			int newItem = ListView_InsertItem(g_hWndListView, &lvItem);

			// Column 2
			LPWSTR lpwstrFileName = PathFindFileName(lpPath);
			lvItem.iSubItem = 1;
			lvItem.pszText = (LPWSTR)lpwstrFileName;
			ListView_SetItem(g_hWndListView, &lvItem);

			// Column 3
			PathRemoveFileSpec(lpPath);
			lvItem.iSubItem = 2;
			lvItem.pszText = (LPWSTR)lpPath;
			ListView_SetItem(g_hWndListView, &lvItem);
			//Column 4
			lvItem.iSubItem = 3;
			wstring sStringPosition = std::to_wstring(i - j);
			LPWSTR pszPosition = new wchar_t[sStringPosition.size() + 1];
			wcscpy_s(pszPosition, sStringPosition.size() + 1, sStringPosition.c_str());
			lvItem.pszText = pszPosition;
			ListView_SetItem(g_hWndListView, &lvItem);
			ListView_EnsureVisible(g_hWndListView, newItem, FALSE);
			delete[] pszIndex;
			delete[] pszPosition;
			//exit if first string found
			return;

			j = lps[j - 1];
		}
		else if (i < textLength && pattern[j] != text[i]) {
			if (j != 0) {
				j = lps[j - 1];
			}
			else {
				i = i + 1;
			}
		}
	}
}

//Stop button handle
void StopButtonHandle() {
	// Check if the search thread handle is not NULL
	if (g_hThreadFindString != NULL && g_bStopFlag == FALSE) {
		// Suspend the execution of the search thread
		SuspendThread(g_hThreadFindString);
		// Enable the search button and text input boxes
		EnableWindow(g_hWndBtnChooseFolder, TRUE);
		EnableWindow(g_hWndBtnSearch, TRUE);
		EnableWindow(g_hWndEdtPathBox, TRUE);
		EnableWindow(g_hWndEdtStringBox, TRUE);
		// Change the text of the search button to "Resume"
		SetWindowText(g_hWndBtnSearch, L"Resume");
		g_bStopFlag = TRUE;
	}
}
