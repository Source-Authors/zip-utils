﻿#include <tchar.h>
#include <windows.h>

#include <cstdio>
#include <vector>
//
#include <commctrl.h>

#include "../../XUnzip.h"
#include "../../XZip.h"

HINSTANCE hInstance;

// This program shows how to display a progress bar.  It first pops up a
// "ZipDialog" with progress bar which zips up some files; then it pops up an
// "UnzipDialog" with progress bar which unzip them.
//
// (1) The UnzipDialog simply invokes the function "UnzipWithProgress".  This
// unzips a zip file chunk by chunk, and the progress bar shows how far through
// the zip file we are.  After every chunk it calls "PumpMessages" to handle any
// outstanding Windows messages (eg. repainting windows, dragging them,
// responding to clicks).  NB.  if the user happened to click Cancel, this will
// be dispatched to the UnzipDialogProc, which responds by setting the global
// flag "abort_p" to true, and in response "UnzipWithProgress" will break out of
// its loop.
INT_PTR CALLBACK UnzipDialogProc(HWND hwnd, UINT msg, WPARAM wParam,
                                 LPARAM lParam);
void UnzipWithProgress(const TCHAR *zipfn, HWND hprog);
void PumpMessages();

bool abort_p = false;

// (2) The ZipDialog -- alas, for adding files to a zip, "zip_utils" provide no
// support for displaying progress.  The call ZipAdd simply doesn't return until
// it's finished.  So what ZipDialogProc does instead is it launches a
// background thread "ZipThreadProc" whose task is to do the zipping. Meanwhile,
// ZipDialogProc starts up a timer, and every 100ms the timer will move the
// progress bar just to show that something's still happening.  When eventually
// the timer discoveres that the thread has finished, it ends the dialog.  If
// you wanted to display real progress, you should look in XZip.cpp/TZip::Add,
// wich calls TZip::ideflate, which calls deflate().  You could conceivably add
// a callback to this.
INT_PTR CALLBACK ZipDialogProc(HWND hwnd, UINT msg, WPARAM wParam,
                               LPARAM lParam);
DWORD WINAPI ZipThreadProc(void *);
HANDLE hThread = nullptr;

// Here's the main code. It just creates a dialog (inline, instead of using a
// resource file) and then invokes ZipDialog and then invokes UnzipDialog.
//
int WINAPI WinMain(_In_ HINSTANCE h, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
  hInstance = h;

  //
#pragma pack(push, 1)
  struct TDlgItemTemplate {
    DWORD s, ex;
    short x, y, cx, cy;
    WORD id;
  };
  struct TDlgTemplate {
    DWORD s, ex;
    WORD cdit;
    short x, y, cx, cy;
  };
  struct TDlgItem1 {
    TDlgItemTemplate dli;
    WCHAR cls[7], tit[7];
    WORD cdat;
  };
  struct TDlgItem2 {
    TDlgItemTemplate dli;
    WCHAR cls[18], tit[1];
    WORD cdat;
  };
  struct TDlgData {
    TDlgTemplate dlt;
    WORD m, c;
    WCHAR t[8];
    WORD pt;
    WCHAR f[14];
    TDlgItem1 i1;
    TDlgItem2 i2;
  };
  TDlgData dtp = {
      {DS_MODALFRAME | DS_3DLOOK | DS_SETFONT | DS_CENTER | WS_POPUP |
           WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
       0, 2, 0, 0, 278, 54},
      0,
      0,
      L"Zipping",
      12,
      L"MS SansSerif",
      {{BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE, 0, 113, 32, 50, 14, IDCANCEL},
       L"BUTTON",
       L"Cancel",
       0},
      {{WS_CHILD | WS_VISIBLE, 0, 7, 7, 264, 18, 1},
       L"msctls_progress32",
       L"",
       0}};
#pragma pack(pop)

  INT_PTR res =
      DialogBoxIndirect(hInstance, (DLGTEMPLATE *)&dtp, 0, ZipDialogProc);
  if (res == IDCANCEL) return 0;

  return static_cast<int>(
      DialogBoxIndirect(hInstance, (DLGTEMPLATE *)&dtp, 0, UnzipDialogProc));
}

INT_PTR CALLBACK UnzipDialogProc(HWND hwnd, UINT msg,
                                 [[maybe_unused]] WPARAM wParam,
                                 [[maybe_unused]] LPARAM lParam) {
  if (msg == WM_INITDIALOG) {
    SetWindowText(hwnd, "Unzipping...");
    PostMessage(hwnd, WM_USER, 0, 0);
    return TRUE;
  }

  if (msg == WM_COMMAND) {
    abort_p = true;
    return TRUE;
  }

  if (msg == WM_USER) {
    UnzipWithProgress("\\z\\progress.zip", GetDlgItem(hwnd, 1));
    EndDialog(hwnd, IDOK);
    return TRUE;
  }

  return FALSE;
}

void UnzipWithProgress(const TCHAR *zipfn, HWND hprog) {
  HZIP hz = OpenZip(zipfn, 0);
  ZIPENTRY ze;
  GetZipItem(hz, -1, &ze);
  int numentries = ze.index;
  // first we retrieve the total size of all zip items
  DWORD tot = 0;
  for (int i = 0; i < numentries; i++) {
    GetZipItem(hz, i, &ze);
    tot += ze.unc_size;
  }
  //
  DWORD countall = 0;  // this is our progress so far
  for (int i = 0; i < numentries && !abort_p; i++) {
    GetZipItem(hz, i, &ze);
    // We'll unzip each file bit by bit, to a file on disk
    char fn[1024];
    _sntprintf(fn, ARRAYSIZE(fn), "\\z\\%s", ze.name);
    HANDLE hf = CreateFile(fn, GENERIC_WRITE, 0, 0, CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL, 0);
    // Each chunk will be 16k big. After each chunk, we show progress
    char buf[16384];
    DWORD countfile = 0;
    // nb. the global "abort_p" flag is set by the user clicking the Cancel
    // button.
    for (ZRESULT zr = ZR_MORE; zr == ZR_MORE && !abort_p;) {
      zr = UnzipItem(hz, i, buf, 16384);
      unsigned long bufsize = 16384;
      if (zr == ZR_OK)
        bufsize =
            ze.unc_size - countfile;  // bufsize is how much we got this time
      DWORD writ;
      WriteFile(hf, buf, bufsize, &writ, 0);
      // countfile counts how much of this file we've unzipped so far
      countfile += bufsize;
      // countall counts how much total we've unzipped so far
      countall += bufsize;
      // Now show progress, and let Windows catch up...
      int percent = (int)(100.0 * ((double)countall) / ((double)tot));
      SendMessage(hprog, PBM_SETPOS, percent, 0);
      PumpMessages();
    }

    CloseHandle(hf);

    if (abort_p) DeleteFile(fn);
  }
  CloseZip(hz);
}

void PumpMessages() {
  for (MSG msg;;) {
    BOOL res = PeekMessage(&msg, 0, 0, 0, PM_REMOVE);
    if (!res || msg.message == WM_QUIT) return;

    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
}

INT_PTR CALLBACK ZipDialogProc(HWND hwnd, UINT msg,
                               [[maybe_unused]] WPARAM wParam,
                               [[maybe_unused]] LPARAM lParam) {
  if (msg == WM_INITDIALOG) {
    EnableWindow(GetDlgItem(hwnd, IDCANCEL), FALSE);
    SetTimer(hwnd, 1, 100, 0);
    hThread = CreateThread(0, 0, ZipThreadProc, 0, 0, 0);
    return TRUE;
  }

  if (msg == WM_TIMER) {
    LRESULT i = SendDlgItemMessage(hwnd, 1, PBM_GETPOS, 0, 0);
    SendDlgItemMessage(hwnd, 1, PBM_SETPOS, (i + 1) % 100, 0);
    if (hThread != 0) {
      DWORD res =
          WaitForSingleObject(hThread, 0);  // has the thread finished yet?
      if (res == WAIT_OBJECT_0) {
        CloseHandle(hThread);
        hThread = 0;
        EndDialog(hwnd, IDOK);
      }
    }
  }

  if (msg == WM_DESTROY) KillTimer(hwnd, 1);

  return FALSE;
}

DWORD WINAPI ZipThreadProc(void *) {
  constexpr unsigned size = 512 * 1024 * 1024;  // 512mb big!
  std::vector<char> c;
  c.reserve(size);
  for (size_t i = 0; i < size; i += 4) {
    int r = rand();

    c.emplace_back(static_cast<char>(r & 0xFF));
    c.emplace_back(static_cast<char>((r >> 8) & 0xFF));
    c.emplace_back(static_cast<char>((r >> 16) & 0xFF));
    c.emplace_back(static_cast<char>((r >> 24) & 0xFF));
  }

  char r = static_cast<char>(rand() & 0xFF);
  for (unsigned i = 0; i < (size & (4 - 1)); ++i) {
    c.emplace_back(r);
  }

  CreateDirectory("\\z", 0);

  HZIP hz = CreateZip("\\z\\progress.zip", 0);
  ZipAdd(hz, "progress1.zip", c.data(), size);
  ZipAdd(hz, "progress2.zip", c.data(), size);
  CloseZip(hz);
  return 0;
}
