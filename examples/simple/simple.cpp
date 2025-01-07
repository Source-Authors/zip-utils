#include <tchar.h>
#include <windows.h>

#include <cstdio>
#include <memory>

#include "../test_utils.h"
#include "XUnzip.h"
#include "XZip.h"

using namespace zu_utils::examples;

namespace {

using zip_ptr = std::unique_ptr<HZIP__, ZipDeleter<msg>>;

DWORD WINAPI Ex3ThreadFunc(void *dat) {
  SetThreadDescription(::GetCurrentThread(), L"Pipe write thread");

  HANDLE hwrite = static_cast<HANDLE>(dat);

  char buf[1000] = {17};
  DWORD writ;

  DWORD rc = 0;
  BOOL ok = WriteFile(hwrite, buf, std::size(buf), &writ, nullptr);
  if (!ok) {
    rc = GetLastError();

    msg(_T("* Failed to write to pipe"));
  }

  if (!CloseHandle(hwrite)) {
    ok = FALSE;

    if (!rc) rc = GetLastError();

    msg(_T("* Failed to close pipe write end"));
  }

  return ok ? 0 : rc;
}

struct BitmapDeleter {
  void operator()(HBITMAP__ *bmp) const noexcept {
    if (!DeleteObject(bmp)) {
      DWORD rc = ::GetLastError();
      msg(_T("* Failed to delete bitmap"));
      return exit(rc);
    }
  }
};

struct ReleaseDcDeleter {
  void operator()(HDC__ *dc) const noexcept {
    if (!ReleaseDC(0, dc)) {
      DWORD rc = ::GetLastError();
      msg(_T("* Failed to release device context"));
      return exit(rc);
    }
  }
};

struct CompatibleDcDeleter {
  void operator()(HDC__ *dc) const noexcept {
    if (!DeleteDC(dc)) {
      DWORD rc = ::GetLastError();
      msg(_T("* Failed to delete device context"));
      return exit(rc);
    }
  }
};

// will create "\\simple.bmp" and "\\simple.txt" for subsequent zipping
BOOL CreateFiles() {
  // Dummy file: bitmap snapshot of screen
  int w = GetSystemMetricsForDpi(SM_CXSCREEN, GetDpiForSystem()),
      h = GetSystemMetricsForDpi(SM_CYSCREEN, GetDpiForSystem());
  int bw = w, bh = h;
  int bw4 = (bw + 3) & 0xFFFFFFFC;

  BITMAPINFO bi = {};
  bi.bmiHeader.biBitCount = 24;
  bi.bmiHeader.biClrImportant = 0;
  bi.bmiHeader.biClrUsed = 0;
  bi.bmiHeader.biCompression = BI_RGB;
  bi.bmiHeader.biHeight = bh;
  bi.bmiHeader.biPlanes = 1;
  bi.bmiHeader.biSize = 40;
  bi.bmiHeader.biSizeImage = bw4 * bh * 3;
  bi.bmiHeader.biWidth = bw4;
  bi.bmiHeader.biXPelsPerMeter = 3780;
  bi.bmiHeader.biYPelsPerMeter = 3780;
  bi.bmiColors[0].rgbBlue = 0;
  bi.bmiColors[0].rgbGreen = 0;
  bi.bmiColors[0].rgbRed = 0;
  bi.bmiColors[0].rgbReserved = 0;

  {
    using rdc_ptr = std::unique_ptr<HDC__, ReleaseDcDeleter>;
    using cdc_ptr = std::unique_ptr<HDC__, CompatibleDcDeleter>;
    using bitmap_ptr = std::unique_ptr<HBITMAP__, BitmapDeleter>;

    rdc_ptr sdc{GetDC(0)};

    BYTE *bits;
    bitmap_ptr hbm{CreateDIBSection(sdc.get(), &bi, DIB_RGB_COLORS,
                                    (void **)&bits, NULL, 0)};
    if (!hbm) msg(_T("Failed to create bitmap"));

    {
      cdc_ptr hdc{CreateCompatibleDC(sdc.get())};

      HGDIOBJ hold = SelectObject(hdc.get(), hbm.get());
      if (hold != nullptr && hold != HGDI_ERROR) {
        StretchBlt(hdc.get(), 0, 0, bw, bh, sdc.get(), 0, 0, w, h, SRCCOPY);

        SelectObject(hdc.get(), hold);
      }
    }

    {
      BITMAPFILEHEADER bfh = {};
      bfh.bfType = 0x4d42;
      bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + bi.bmiHeader.biSize;
      bfh.bfSize = bfh.bfOffBits + bi.bmiHeader.biSizeImage;
      bfh.bfReserved1 = 0;
      bfh.bfReserved2 = 0;

      handle_ptr<msg> hf{CreateFile(_T("\\simple.bmp"), GENERIC_WRITE, 0, NULL,
                                    CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                                    NULL)};
      if (!hf) msg(_T("* Failed to create '\\simple.bmp'"));

      DWORD writ;
      bool rc =
          WriteFile(hf.get(), &bfh, sizeof(bfh), &writ, NULL) &&
          WriteFile(hf.get(), &bi.bmiHeader, sizeof(bi.bmiHeader), &writ,
                    NULL) &&
          WriteFile(hf.get(), bits, bi.bmiHeader.biSizeImage, &writ, NULL);

      if (rc) {
        msg(_T("Created '\\simple.bmp'"));
      } else {
        msg(_T("* Failed to write '\\simple.bmp'"));
      }
    }

    {
      //
      // Dummy file: text
      constexpr char src[] =
          "Few things are more distressing\r\nto a well regulated mind\r\nthan "
          "to "
          "see a boy who ought to know better\r\ndisporting himself at "
          "improper "
          "moments.\r\n";

      handle_ptr<msg> hf{CreateFile(_T("\\simple.txt"), GENERIC_WRITE, 0, NULL,
                                    CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                                    NULL)};
      if (!hf) msg(_T("* Failed to create '\\simple.txt'"));

      DWORD writ;
      BOOL rc = WriteFile(hf.get(), src, (DWORD)strlen(src), &writ, NULL);

      if (rc)
        msg(_T("Created '\\simple.txt'"));
      else
        msg(_T("* Failed to write '\\simple.txt'"));
    }
  }

  return any_errors ? FALSE : TRUE;
}

}  // namespace

int main() {
  BOOL crc = CreateFiles();
  if (!crc) msg(_T("* Failed to create files"));

  {
    // EXAMPLE 1 - create a zipfile from existing files
    zip_ptr hz{CreateZip(_T("\\simple1.zip"), 0)};
    if (!hz) msg(_T("* Failed to create \\simple1.zip"));

    ZRESULT rc = ZipAdd(hz.get(), _T("znsimple.bmp"), _T("\\simple.bmp"));
    if (rc != ZR_OK) msg(_T("* Failed to add simple.bmp to \\simple1.zip"));

    rc = ZipAdd(hz.get(), _T("znsimple.txt"), _T("\\simple.txt"));
    if (rc != ZR_OK) msg(_T("* Failed to add simple.txt to \\simple1.zip"));

    if (rc == ZR_OK) msg(_T("Created '\\simple1.zip'"));
  }

  {
    // EXAMPLE 2 - unzip it with the names suggested in the zip
    zip_ptr hz{OpenZip(_T("\\simple1.zip"), 0)};
    ZRESULT rc = SetUnzipBaseDir(hz.get(), _T("\\"));
    if (rc != ZR_OK) {
      msg(_T("* Failed to set unzip base dir for \\simple1.zip"));
    }

    ZIPENTRY ze;
    rc = GetZipItem(hz.get(), -1, &ze);
    if (rc != ZR_OK) {
      msg(_T("* Failed to get items count for zip \\simple1.zip"));
    }

    int numitems = ze.index;
    for (int zi = 0; zi < numitems; zi++) {
      rc = GetZipItem(hz.get(), zi, &ze);
      if (rc != ZR_OK) {
        msg(_T("* Failed to get N item for zip \\simple1.zip"));
      }

      rc = UnzipItem(hz.get(), zi, ze.name);
      if (rc != ZR_OK) {
        msg(_T("* Failed to unzip N item for zip \\simple1.zip"));
      }
    }

    if (rc == ZR_OK) {
      msg(_T("Unzipped 'znsimple.bmp' and 'znsimple.txt' from 'simple1.zip'"));
    }
  }

  {
    // EXAMPLE 3 - create an auto-allocated pagefile-based zip file from various
    // sources the second argument says how much address space to reserve for
    // it.  We can afford to be generous: no address-space is allocated unless
    // it's actually needed.
    zip_ptr hz{CreateZip(0, 100000, "password")};
    if (!hz) msg(_T("* Failed to create ppassword protected zip"));

    // adding a conventional file...
    ZRESULT rc = ZipAdd(hz.get(), _T("znsimple.txt"), _T("\\simple.txt"));
    if (rc != ZR_OK) {
      msg(_T("* Failed to add simple.txt to password protected zip"));
    }

    // adding something from memory...
    char buf[1000];
    for (size_t zj = 0; zj < std::size(buf); zj++) buf[zj] = (char)(zj & 0x7F);

    rc = ZipAdd(hz.get(), _T("simple.dat"), buf, 1000);
    if (rc != ZR_OK) {
      msg(_T("* Failed to add simple.dat to password protected zip"));
    }

    // adding something from a pipe...
    handle_ptr<msg> hread;
    HANDLE hwrite = nullptr;
    if (CreatePipe(&hread, &hwrite, NULL, 0)) {
      // hwrite closed by thread.
      handle_ptr<msg> hthread{CreateThread(0, 0, Ex3ThreadFunc, hwrite, 0, 0)};
      if (hthread) {
        // the '1000' is optional, but it makes for nicer zip
        // files if sizes are known in advance.
        rc = ZipAddHandle(hz.get(), _T("simple3.dat"), hread.get(), 1000);
        if (rc != ZR_OK) {
          msg(_T("* Failed to add simple3.dat from pipe to password protected ")
              _T("zip"));
        }

        WaitForSingleObject(hthread.get(), INFINITE);
      } else {
        // Close hwrite.
        if (!CloseHandle(hwrite)) msg(_T("* Failed to close pipe write end"));

        msg(_T("* Failed to create thread"));
      }
    } else {
      msg(_T("* Failed to create pipe"));
    }

    //
    // and now that the zip is created in pagefile, let's do something with it:
    void *zbuf;
    unsigned long zlen;
    rc = ZipGetMemory(hz.get(), &zbuf, &zlen);
    if (rc != ZR_OK) msg(_T("* Failed to get zip memory"));

    handle_ptr<msg> hfz{CreateFile(_T("\\simple3 - pwd is 'password'.zip"),
                                   GENERIC_WRITE, 0, 0, CREATE_ALWAYS,
                                   FILE_ATTRIBUTE_NORMAL, 0)};
    if (!hfz) {
      msg(_T("* Failed to create zip '\\simple3 - pwd is 'password'.zip'"));
    }

    DWORD writ;
    BOOL wrc = WriteFile(hfz.get(), zbuf, zlen, &writ, NULL);

    if (wrc)
      msg(_T("Created '\\simple3 - pwd is 'password'.zip' via pagefile"));
    else
      msg(_T("* Failed to write 'zip \\simple3 - pwd is 'password'.zip'"));
  }

  {
    // EXAMPLE 4 - unzip directly from resource into a file
    // resource RT_RCDATA/#1 happens to be a zip file...
    HINSTANCE hInstance = GetModuleHandle(0);
    HRSRC hrsrc = FindResource(hInstance, MAKEINTRESOURCE(1), RT_RCDATA);
    if (!hrsrc) msg(_T("* Failed to find 1 resource"));

    HANDLE hglob = LoadResource(hInstance, hrsrc);
    if (!hglob) msg(_T("* Failed to load 1 resource"));

    void *zipbuf = LockResource(hglob);
    if (!zipbuf) msg(_T("* Failed to lock 1 resource"));

    unsigned int ziplen = SizeofResource(hInstance, hrsrc);
    if (!ziplen) msg(_T("* Failed to get size of 1 resource"));

    zip_ptr hz{OpenZip(zipbuf, ziplen, 0)};
    if (!hz) msg(_T("* Failed to create zip in memory"));

    ZRESULT rc = SetUnzipBaseDir(hz.get(), _T("\\"));
    if (rc != ZR_OK) msg(_T("* Failed to set unzip base dir to '\\\\'"));

    int i;
    ZIPENTRY ze;
    rc = FindZipItem(hz.get(), _T("simple.jpg"), true, &i, &ze);
    if (rc != ZR_OK) msg(_T("* Failed to find 'simple.jpg' in zip"));

    //   - unzip to a file -
    rc = UnzipItem(hz.get(), i, ze.name);
    if (rc != ZR_OK) msg(_T("* Failed to unzip 'simple.jpg' to file"));

    {
      //   - unzip to a membuffer -
      std::unique_ptr<char[]> ibuf{std::make_unique<char[]>(ze.unc_size)};

      rc = UnzipItem(hz.get(), i, ibuf.get(), ze.unc_size);
      if (rc != ZR_OK) {
        msg(_T("* Failed to unzip 'simple.jpg' to heap memory buffer"));
      }
    }

    //   - unzip to a fixed membuff, bit by bit -
    char fbuf[1024];
    rc = ZR_MORE;
    unsigned long totsize = 0;

    while (rc == ZR_MORE) {
      rc = UnzipItem(hz.get(), i, fbuf, std::size(fbuf));
      if (rc != ZR_OK && rc != ZR_MORE) {
        msg(_T("* Failed to unzip 'simple.jpg' to stack memory buffer"));
      }

      unsigned long bufsize = 1024;
      if (rc == ZR_OK) bufsize = ze.unc_size - totsize;

      // now do something with this chunk which we just unzipped...
      totsize += bufsize;
    }

    if (rc == ZR_OK)
      // note: no need to free resources obtained through Find/Load/LockResource
      msg(_T("Unzipped 'simple.jpg' to stack memory buffer from resource ")
          _T("zipfile"));
    else
      msg(_T("* Failed to unzip 'simple.jpg' to stack memory buffer from ")
          _T("resource zipfile"));
  }

  if (IsFileExist(_T("\\simple.txt"))) {
    if (!DeleteFile(_T("\\simple.txt"))) {
      msg(_T("* Failed to delete file '\\simple.txt'"));
    } else {
      msg(_T("Deleted file '\\simple.txt'"));
    }
  }

  if (IsFileExist(_T("\\simple.bmp"))) {
    if (!DeleteFile(_T("\\simple.bmp"))) {
      msg(_T("* Failed to delete file '\\simple.bmp'"));
    } else {
      msg(_T("Deleted file '\\simple.bmp'"));
    }
  }

  if (any_errors) {
    msg(_T("Finished"));
    return 1;
  }

  msg(_T("Finished. No errors."));
  return 0;
}
