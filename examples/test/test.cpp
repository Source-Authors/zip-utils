// This program runs a bunch of test cases on zip/unzip.
//
// Most of the test cases come from bugs that had been reported in an earlier
// version of the ziputils, to be sure that they're fixed.

#include <tchar.h>
#include <windows.h>

#include <cstdio>
#include <memory>
#include <vector>

#include "../test_utils.h"
#include "XUnzip.h"
#include "XZip.h"

using namespace zu_utils::examples;

using zip_ptr = std::unique_ptr<HZIP__, ZipDeleter<msg>>;

namespace {

bool tsame(const FILETIME t0, const FILETIME t1) noexcept {
  if (t0.dwHighDateTime != t1.dwHighDateTime) return false;
  if ((t0.dwLowDateTime >> 28) != (t1.dwLowDateTime >> 28)) return false;

  // We allow some flexibility in the lower bits.  That's because zip's don't
  // store times with as much precision.
  return true;
}

BOOL SaveResource(const TCHAR *res, const TCHAR *fn) {
  HINSTANCE hInstance = GetModuleHandle(0);
  if (!hInstance) return FALSE;

  HRSRC hrsrc = FindResource(hInstance, res, RT_RCDATA);
  if (!hrsrc) return FALSE;

  HANDLE hglob = LoadResource(hInstance, hrsrc);
  if (!hglob) return FALSE;

  void *buf = LockResource(hglob);
  const DWORD len = SizeofResource(hInstance, hrsrc);
  handle_ptr<msg> hf{CreateFile(fn, GENERIC_WRITE, 0, 0, CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_ARCHIVE |
                                    FILE_FLAG_BACKUP_SEMANTICS |
                                    FILE_FLAG_SEQUENTIAL_SCAN,
                                0)};
  if (!hf) return FALSE;

  DWORD writ;
  return WriteFile(hf.get(), buf, len, &writ, 0);
}

}  // namespace

int main() {
  DWORD writ;
  ZIPENTRY ze;
  TCHAR m[1024];

  msg(_T("Zip-utils tests. Files will be left in \"\\z\""));
  if (!CreateDirectory(_T("\\z"), 0) &&
      ::GetLastError() != ERROR_ALREADY_EXISTS)
    msg(_T("* Failed to create directory \\z"));

  if (!SaveResource(MAKEINTRESOURCE(1), _T("\\z\\extra.zip")))
    msg(_T("* Failed to extract resouce to \\z\\extra.zip"));

  if (!SaveResource(MAKEINTRESOURCE(2), _T("\\z\\ce2ce.jpg")))
    msg(_T("* Failed to extract resouce to \\z\\ce2ce.jpg"));

  if (!SaveResource(MAKEINTRESOURCE(3), _T("\\z\\ce2ce.txt")))
    msg(_T("* Failed to extract resouce to \\z\\ce2ce.txt"));

  // fixed bug: OpenZip errors and returns0 when you try to open a zip with no
  // files in it
  msg(_T("empty - testing whether it fails to open empty zipfiles"));
  HZIP hz = CreateZip(_T("\\z\\empty.zip"), 0);
  if (hz == 0) msg(_T("* Failed to create empty.zip"));
  ZRESULT zr = CloseZip(hz);
  if (zr != ZR_OK) msg(_T("* Failed to close empty.zip"));
  if (p_abort) return 1;
  hz = OpenZip(_T("\\z\\empty.zip"), 0);
  if (hz == 0) msg(_T("* Failed to open empty.zip"));
  zr = GetZipItem(hz, -1, &ze);
  if (zr != ZR_OK) msg(_T("* Failed to get empty.zip index"));
  zr = CloseZip(hz);
  if (zr != ZR_OK) msg(_T("* Failed to close empty.zip"));
  if (p_abort) return 1;

  // fixed bug: IsZipHandle should return false for a NULL handle.
  msg(_T("IsZipHandle - testing whether 0 is considered a handle"));

  bool b = IsZipHandleZ(0) || IsZipHandleU(0);
  if (b) msg(_T("IsZipHandle failed to deny handlehood of NULL"));
  if (p_abort) return 1;

  // fixed bug: if one file is bigger then the following must be smaller than
  // 64k -- diff. between release and debug mode fixed bug: test0=71k,
  // test1=152.2k, test2=145b, test3=120k, here test3 returns ZR_WRITE fixed
  // bug: when adding multiple large files only the first file greater than ~64K
  // is added successfully. Each successive file will only be added if it is
  // less than ~64k. After more testing the exact byte size varies slightly.
  // (might depend on the compression ratio). For example a text file with ~64K
  // of the letter 'a' will fail at exactly 65,276Bytes (File size not Size on
  // Disk)However if the file is mixed letters 'asdjkl' then it will fail
  // consistently at 65,483Bytes. REPRO STEPS: create two text files that are
  // 67K. (To mimick my exact test just use the letter 'a'). Only the first one
  // will be added. When you attempt to add another file that is > ~64K zipAdd
  // Returns ERROR 0x00000400 0xZR_WRITE "a general error writing to the file".
  // Again there is no problem adding additional files if they are <~64K. ERROR
  // ANALYSIS: This is caused by deflate(TXtate &state) setting state.err to
  // "insufficient lookahead"
  msg(_T("sizes - testing whether large-then-small files work okay"));
  {
    std::vector<char> c;
    c.reserve(200U * 1024);
    for (size_t i = 0; i < c.capacity(); i++)
      c.emplace_back((char)(rand() % 255));

    {
      handle_ptr<msg> hf{CreateFile(
          _T("\\z\\sizes-71k.dat"), GENERIC_WRITE, 0, 0, CREATE_ALWAYS,
          FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_ARCHIVE |
              FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_SEQUENTIAL_SCAN,
          0)};
      if (!hf) msg(_T("* Failed to create \\z\\sizes-71k.dat"));

      if (!WriteFile(hf.get(), c.data(), 71 * 1024, &writ, 0)) {
        msg(_T("* Failed to write \\z\\sizes-71k.dat"));
      }
    }

    {
      handle_ptr<msg> hf{CreateFile(
          _T("\\z\\sizes-152_2k.dat"), GENERIC_WRITE, 0, 0, CREATE_ALWAYS,
          FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_ARCHIVE |
              FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_SEQUENTIAL_SCAN,
          0)};
      if (!hf) msg(_T("* Failed to create \\z\\sizes-152_2k.dat"));

      if (!WriteFile(hf.get(), c.data(), 152 * 1024 + 1024 / 5, &writ, 0)) {
        msg(_T("* Failed to write \\z\\sizes-152_2k.dat"));
      }
    }

    {
      handle_ptr<msg> hf{CreateFile(
          _T("\\z\\sizes-145b.dat"), GENERIC_WRITE, 0, 0, CREATE_ALWAYS,
          FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_ARCHIVE |
              FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_SEQUENTIAL_SCAN,
          0)};
      if (!hf) msg(_T("* Failed to create \\z\\sizes-145b.dat"));

      if (!WriteFile(hf.get(), c.data(), 145, &writ, 0)) {
        msg(_T("* Failed to write \\z\\sizes-145b.dat"));
      }
    }

    {
      handle_ptr<msg> hf{CreateFile(
          _T("\\z\\sizes-120k.dat"), GENERIC_WRITE, 0, 0, CREATE_ALWAYS,
          FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_ARCHIVE |
              FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_SEQUENTIAL_SCAN,
          0)};
      if (!hf) msg(_T("* Failed to create \\z\\sizes-120k.dat"));

      if (!WriteFile(hf.get(), c.data(), 120 * 1024, &writ, 0)) {
        msg(_T("* Failed to write \\z\\sizes-120k.dat"));
      }
    }
  }

  //
  hz = CreateZip(_T("\\z\\sizes.zip"), 0);
  if (hz == 0) msg(_T("* Failed to create sizes.zip"));
  zr = ZipAdd(hz, _T("sizes-71k.out.dat"), _T("\\z\\sizes-71k.dat"));
  if (zr != ZR_OK) msg(_T("* Failed to add 71k"));
  zr = ZipAdd(hz, _T("sizes-152_2k.out.dat"), _T("\\z\\sizes-152_2k.dat"));
  if (zr != ZR_OK) msg(_T("* Failed to add 152_2k"));
  zr = ZipAdd(hz, _T("sizes-145b.out.dat"), _T("\\z\\sizes-145b.dat"));
  if (zr != ZR_OK) msg(_T("* Failed to add 145b"));
  zr = ZipAdd(hz, _T("sizes-120k.out.dat"), _T("\\z\\sizes-120k.dat"));
  if (zr != ZR_OK) msg(_T("* Failed to add 120k"));
  zr = CloseZip(hz);
  if (zr != ZR_OK) msg(_T("* Failed to close sizes.zip"));
  if (p_abort) return 1;
  hz = OpenZip(_T("\\z\\sizes.zip"), 0);
  if (hz == 0) msg(_T("* Failed to open sizes.zip"));
  zr = UnzipItem(hz, 0, _T("\\z\\sizes-71k.out.dat"));
  if (zr != ZR_OK) msg(_T("* Failed to unzip 71k"));
  if (!fsame<msg>(_T("\\z\\sizes-71k.dat"), _T("\\z\\sizes-71k.out.dat")))
    msg(_T("* Failed to unzip all of 71k"));
  zr = UnzipItem(hz, 1, _T("\\z\\sizes-152_2k.out.dat"));
  if (zr != ZR_OK ||
      !fsame<msg>(_T("\\z\\sizes-152_2k.dat"), _T("\\z\\sizes-152_2k.out.dat")))
    msg(_T("* Failed to unzip 152_2k"));
  zr = UnzipItem(hz, 2, _T("\\z\\sizes-145b.out.dat"));
  if (zr != ZR_OK ||
      !fsame<msg>(_T("\\z\\sizes-145b.dat"), _T("\\z\\sizes-145b.out.dat")))
    msg(_T("* Failed to unzip 145b"));
  zr = UnzipItem(hz, 3, _T("\\z\\sizes-120k.out.dat"));
  if (zr != ZR_OK ||
      !fsame<msg>(_T("\\z\\sizes-120k.dat"), _T("\\z\\sizes-120k.out.dat")))
    msg(_T("* Failed to unzip 120k"));
  zr = CloseZip(hz);
  if (zr != ZR_OK) msg(_T("* Failed to close sizes.zip"));
  if (p_abort) return 1;
  //
  {
    char c[67 * 1024];
    memset(c, 'a', sizeof(c));

    {
      handle_ptr<msg> hf{CreateFile(
          _T("\\z\\sizes2-67k1.dat"), GENERIC_WRITE, 0, 0, CREATE_ALWAYS,
          FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_ARCHIVE |
              FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_SEQUENTIAL_SCAN,
          0)};
      if (!hf) msg(_T("* Failed to create \\z\\sizes2-67k1.dat"));

      if (!WriteFile(hf.get(), c, 67 * 1024, &writ, 0)) {
        msg(_T("* Failed to write \\z\\sizes2-67k1.dat"));
      }
    }

    {
      handle_ptr<msg> hf{CreateFile(
          _T("\\z\\sizes2-67k2.dat"), GENERIC_WRITE, 0, 0, CREATE_ALWAYS,
          FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_ARCHIVE |
              FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_SEQUENTIAL_SCAN,
          0)};
      if (!hf) msg(_T("* Failed to create \\z\\sizes2-67k2.dat"));

      if (!WriteFile(hf.get(), c, 67 * 1024, &writ, 0)) {
        msg(_T("* Failed to write \\z\\sizes2-67k2.dat"));
      }
    }

    hz = CreateZip(_T("\\z\\sizes2.zip"), 0);
    if (hz == 0) msg(_T("* Failed to create sizes2.zip"));
    zr = ZipAdd(hz, _T("sizes2-67k1.out.dat"), _T("\\z\\sizes2-67k1.dat"));
    if (zr != ZR_OK) msg(_T("* Failed to add 67k1"));
    zr = ZipAdd(hz, _T("sizes2-67k2.out.dat"), _T("\\z\\sizes2-67k2.dat"));
    if (zr != ZR_OK) msg(_T("* Failed to add 67k2"));
    zr = CloseZip(hz);
    if (zr != ZR_OK) msg(_T("* Failed to close sizes.zip"));
    if (p_abort) return 1;
    hz = OpenZip(_T("\\z\\sizes2.zip"), 0);
    if (hz == 0) msg(_T("* Failed to open sizes2.zip"));
    zr = UnzipItem(hz, 0, _T("\\z\\sizes2-67k1.out.dat"));
    if (zr != ZR_OK ||
        !fsame<msg>(_T("\\z\\sizes2-67k1.dat"), _T("\\z\\sizes2-67k1.out.dat")))
      msg(_T("* Failed to unzip all of 67k1"));
    zr = UnzipItem(hz, 1, _T("\\z\\sizes2-67k2.out.dat"));
    if (zr != ZR_OK ||
        !fsame<msg>(_T("\\z\\sizes2-67k2.dat"), _T("\\z\\sizes2-67k1.out.dat")))
      msg(_T("* Failed to unzip all of 67k2"));
    zr = CloseZip(hz);
    if (zr != ZR_OK) msg(_T("* Failed to close sizes2.zip"));
    if (p_abort) return 1;
  }

  // fixed bug: if file-size is multiple of internal bufzip, 16k, then last
  // buffer is not saved. Due to unzReadCurrentFile. When buf full and EOF is
  // true at same time, EOF is returned. Could have extra parameter in
  // unzReadCurrentFile. fixed bug: Otherwise unzipping into memory never works!
  // (always fails, returns 0). if (err==Z_STREAM_END) return
  // (iRead==0)?UNSZ_EOF:iRead; -->  if (err==Z_STREAM_END) return
  // (iRead==len)?UNZ_EOF:iRead.
  msg(_T("unztomem - testing whether ZR_OK is returned upon successfull unzip ")
      _T("to memory"));
  {
    std::vector<char> c;
    c.reserve(10240);
    for (size_t i = 0; i < c.capacity(); i++)
      c.emplace_back((char)(rand() % 255));

    {
      handle_ptr<msg> hf{CreateFile(
          _T("\\z\\unztomem-10k24.dat"), GENERIC_WRITE, 0, 0, CREATE_ALWAYS,
          FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_ARCHIVE |
              FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_SEQUENTIAL_SCAN,
          0)};
      if (!hf) msg(_T("* Failed to create \\z\\unztomem-10k24.dat"));

      if (!WriteFile(hf.get(), c.data(), 10240, &writ, 0)) {
        msg(_T("* Failed to write \\z\\unztomem-10k24.dat"));
      }
    }

    {
      handle_ptr<msg> hf{CreateFile(
          _T("\\z\\unztomem-10k00.dat"), GENERIC_WRITE, 0, 0, CREATE_ALWAYS,
          FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_ARCHIVE |
              FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_SEQUENTIAL_SCAN,
          0)};
      if (!hf) msg(_T("* Failed to create \\z\\unztomem-10k00.dat"));

      if (!WriteFile(hf.get(), c.data(), 10000, &writ, 0)) {
        msg(_T("* Failed to write \\z\\unztomem-10k00.dat"));
      }
    }
  }

  hz = CreateZip(_T("\\z\\unztomem.zip"), 0);
  if (hz == 0) msg(_T("* Failed to create unztomem.zip"));
  zr = ZipAdd(hz, _T("unztomem-10k24.dat"), _T("\\z\\unztomem-10k24.dat"));
  if (zr != ZR_OK) msg(_T("* Failed to add unztomem-10k24.dat"));
  zr = ZipAdd(hz, _T("unztomem-10k00.dat"), _T("\\z\\unztomem-10k00.dat"));
  if (zr != ZR_OK) msg(_T("* Failed to add unztomem-10k00.dat"));
  zr = CloseZip(hz);
  if (zr != ZR_OK) msg(_T("* Failed to close unztomem.zip"));
  if (p_abort) return 1;

  // test unzipping a round multiple into smaller buffer
  {
    hz = OpenZip(_T("\\z\\unztomem.zip"), 0);
    if (hz == 0) msg(_T("* Failed to open unztomem.zip"));

    zr = GetZipItem(hz, 0, &ze);
    if (zr != ZR_OK) msg(_T("* Failed to get 0 zip item from unztomem.zip"));

    char buf[1024];
    zr = ZR_MORE;
    long totsize = 0;
    while (zr == ZR_MORE) {
      zr = UnzipItem(hz, 0, buf, 1024);
      if (zr != ZR_MORE && zr != ZR_OK)
        msg(_T("* Failed to unzip a buffer of unztomem-10k24"));
      long curbuf = (zr == ZR_MORE) ? 1024 : ze.unc_size - totsize;
      totsize += curbuf;
    }
    zr = CloseZip(hz);
    if (zr != ZR_OK) msg(_T("* Failed to close unztomem.zip"));
    if (totsize != ze.unc_size)
      msg(_T("* Failed to unzip all of unztomem-10k24 to mem"));
  }
  if (p_abort) return 1;
  // unzip a non-round multiple into a smaller buffer
  {
    hz = OpenZip(_T("\\z\\unztomem.zip"), 0);
    if (hz == 0) msg(_T("* Failed to open unztomem.zip"));
    zr = GetZipItem(hz, 1, &ze);
    if (zr != ZR_OK) msg(_T("* Failed to get 1 zip item from unztomem.zip"));

    char buf[1024];
    zr = ZR_MORE;
    long totsize = 0;
    while (zr == ZR_MORE) {
      zr = UnzipItem(hz, 1, buf, 1024);
      if (zr != ZR_MORE && zr != ZR_OK)
        msg(_T("* Failed to unzip a buffer of unztomem-10k00.zip"));
      long curbuf = (zr == ZR_MORE) ? 1024 : ze.unc_size - totsize;
      totsize += curbuf;
    }
    zr = CloseZip(hz);
    if (zr != ZR_OK) msg(_T("* Failed to close unztomem.zip"));
    if (totsize != ze.unc_size)
      msg(_T("* Failed to unzip all of unztomem-10k00 to mem"));
  }
  if (p_abort) return 1;
  // unzip a round multiple into a bigger buffer
  {
    hz = OpenZip(_T("\\z\\unztomem.zip"), 0);
    if (hz == 0) msg(_T("* Failed to open unztomem.zip"));
    char buf[16384];
    zr = UnzipItem(hz, 0, buf, 16384);
    if (zr != ZR_OK) msg(_T("* Failed to unzip the buffer of unztomem-10k24"));
    zr = CloseZip(hz);
    if (zr != ZR_OK) msg(_T("* Failed to close unztomem.zip"));
  }
  if (p_abort) return 1;
  // unzip a nonround multiple into a bigger buffer
  {
    hz = OpenZip(_T("\\z\\unztomem.zip"), 0);
    if (hz == 0) msg(_T("* Failed to open unztomem.zip"));
    char buf[16384];
    zr = UnzipItem(hz, 0, buf, 16384);
    if (zr != ZR_OK) msg(_T("* Failed to unzip the buffer of unztomem-10k00"));
    zr = CloseZip(hz);
    if (zr != ZR_OK) msg(_T("* Failed to close unztomem.zip"));
  }
  if (p_abort) return 1;

  // fixed bug: doesn't work on ARM, change lutime_t mtime =
  // *(lutime_t*)(extra+epos); epos+=4; to lutime_t mtime;
  // memcpy(&mtime,&extra[epos],4); epos+=4;. Also in TUnzip::Get, such atime
  // and ctime fixed bug: (extrapos+epos) has size 4 while sizeof(lutime_t) is
  // currently 8
  msg(_T("time - testing whether timestamps are preserved on CE"));
  {
    {
      constexpr TCHAR c[] = _T("Time waits for no man\r\n");

      handle_ptr<msg> hf{
          CreateFile(_T("\\z\\time.txt"), GENERIC_WRITE, 0, 0, CREATE_ALWAYS,
                     FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_ARCHIVE |
                         FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_SEQUENTIAL_SCAN,
                     0)};
      if (!hf) msg(_T("* Failed to create \\z\\time.txt"));

      if (!WriteFile(hf.get(), c, (DWORD)(_tcslen(c) * sizeof(TCHAR)), &writ,
                     0)) {
        msg(_T("* Failed to write \\z\\time.txt"));
      }
    }

    BY_HANDLE_FILE_INFORMATION fi;
    {
      handle_ptr<msg> hf{CreateFile(_T("\\z\\time.txt"), GENERIC_READ,
                                    FILE_SHARE_READ, 0, OPEN_EXISTING,
                                    FILE_FLAG_SEQUENTIAL_SCAN, 0)};
      if (!hf) msg(_T("* Failed to read \\z\\time.txt"));

      if (!GetFileInformationByHandle(hf.get(), &fi)) {
        msg(_T("* Failed to get info by handle to \\z\\time.txt"));
      }
    }

    hz = CreateZip(_T("\\z\\time.zip"), 0);
    if (hz == 0) msg(_T("* Failed to create hello.zip"));
    zr = ZipAdd(hz, _T("time.txt"), _T("\\z\\time.txt"));
    if (zr != ZR_OK) msg(_T("* Failed to add time.txt"));
    zr = CloseZip(hz);
    if (zr != ZR_OK) msg(_T("* Failed to close time.zip"));
    if (p_abort) return 1;
    hz = OpenZip(_T("\\z\\time.zip"), 0);
    if (hz == 0) msg(_T("* Failed to open time.zip"));
    zr = GetZipItem(hz, 0, &ze);
    if (zr != ZR_OK) msg(_T("* Failed to GetZipItem(time.txt)"));
    zr = CloseZip(hz);
    if (zr != ZR_OK) msg(_T("* Failed to close time.zip"));
    if (!tsame(fi.ftCreationTime, ze.ctime))
      msg(_T("* Failed to preserve creation time of hello.txt"));
    if (!tsame(fi.ftLastWriteTime, ze.mtime))
      msg(_T("* Failed to preserve modify time of hello.txt"));
    if (!tsame(fi.ftLastAccessTime, ze.atime))
      msg(_T("* Failed to preserve access time of hello.txt"));
    if (p_abort) return 1;
  }

  // fixed bug: Some problem occurs in the library when reading the "extra
  // field" removing the "extra field" the GetZipItem function functions.  Some
  // solution? fixed bug: unzipping a password-zipfile. Seems to miss 12chars
  // from the original file. Line 3530:
  // pfile_in_zip_read_info->rest_read_uncompressed-=uDoEncHead. Is this right?
  msg(_T("extra - testing whether the extra headers cause problems, with ")
      _T("passwords"));
  if (GetFileAttributes(_T("\\z\\extra.zip")) != INVALID_FILE_ATTRIBUTES) {
    hz = OpenZip(_T("\\z\\extra.zip"), 0);
    if (hz == 0) msg(_T("extra.zip is absent; can't do this test"));
    zr = GetZipItem(hz, 0, &ze);
    if (zr != ZR_OK) msg(_T("* Failed to GetItem(extra.txt)"));
    zr = UnzipItem(hz, 0, _T("\\z\\extra.txt"));
    if (zr != ZR_OK) msg(_T("* Failed to UnzipItem(extra.txt)"));
    zr = CloseZip(hz);
    if (zr != ZR_OK) msg(_T("* Failed to close extra.zip"));
    if (p_abort) return 1;

    handle_ptr<msg> hf{CreateFile(_T("\\z\\extra.txt"), GENERIC_READ,
                                  FILE_SHARE_READ, 0, OPEN_EXISTING,
                                  FILE_FLAG_SEQUENTIAL_SCAN, 0)};
    if (!hf) msg(_T("* Failed to read \\z\\extra.txt"));

    LARGE_INTEGER size = {};
    if (!GetFileSizeEx(hf.get(), &size)) {
      msg(_T("* Failed to get size of \\z\\extra.txt"));
    }

    if (size.QuadPart != ze.unc_size)
      msg(_T("* Failed to unzip all of extra.txt"));

    if (p_abort) return 1;
  }
  {
    {
      constexpr TCHAR c[] =
          _T("Hello there this will have to be a very long file if I want to ")
          _T("see what I want to see the quick brown fox jumped over the lazy ")
          _T("dog and a merry time was had by all at the party few things are ")
          _T("more distressing to a well-regulated mind than to see a boy who ")
          _T("ought to know better disporting himself at improper moments\r\n");

      handle_ptr<msg> hf{
          CreateFile(_T("\\z\\extrapp.txt"), GENERIC_WRITE, 0, 0, CREATE_ALWAYS,
                     FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_ARCHIVE |
                         FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_SEQUENTIAL_SCAN,
                     0)};
      if (!hf) msg(_T("* Failed to write \\z\\extrapp.txt"));

      if (!WriteFile(hf.get(), c, (DWORD)(_tcslen(c) * sizeof(TCHAR)), &writ,
                     0)) {
        msg(_T("* Failed to write \\z\\extrapp.txt"));
      }
    }

    hz = CreateZip(_T("\\z\\extrapp.zip"), "password");
    if (hz == 0) msg(_T("* Failed to create extrapp.zip"));
    zr = ZipAdd(hz, _T("extrapp.txt"), _T("\\z\\extrapp.txt"));
    if (zr != ZR_OK) msg(_T("* Failed to add extrapp.txt"));
    zr = CloseZip(hz);
    if (zr != ZR_OK) msg(_T("* Failed to close extrapp.zip"));
    if (p_abort) return 1;
    hz = OpenZip(_T("\\z\\extrapp.zip"), "password");
    if (hz == 0) msg(_T("* Failed to open extrapp.zip"));
    zr = GetZipItem(hz, 0, &ze);
    if (zr != ZR_OK) msg(_T("* Failed to GetItem(extrapp.txt)"));
    zr = UnzipItem(hz, 0, _T("\\z\\extrapp.out.txt"));
    if (zr != ZR_OK) msg(_T("* Failed to unzip extrapp.txt"));
    zr = CloseZip(hz);
    if (zr != ZR_OK) msg(_T("* Failed to close extrapp.zip"));
    if (!fsame<msg>(_T("\\z\\extrapp.txt"), _T("\\z\\extrapp.out.txt")))
      msg(_T("* Failed to unzip all of extrapp.txt"));
    if (p_abort) return 1;
  }

  // fixed bug: XP(zip)->CE(unzip) works, and Winzip->CE(unzip) works, but
  // CE(zip)->CE(unzip) doesn't.
  msg(_T("ce2ce - testing whether ce zipping and unzipping work"));
  if (GetFileAttributes(_T("\\z\\ce2ce.jpg")) != INVALID_FILE_ATTRIBUTES &&
      GetFileAttributes(_T("\\z\\ce2ce.txt")) != INVALID_FILE_ATTRIBUTES) {
    hz = CreateZip(_T("\\z\\ce2ce.zip"), 0);
    if (hz == 0) msg(_T("* Failed to create ce2ce.zip"));
    zr = ZipAdd(hz, _T("ce2ce.out.jpg"), _T("\\z\\ce2ce.jpg"));
    if (zr != ZR_OK) msg(_T("* Failed to zip ce2ce.jpg"));
    zr = ZipAdd(hz, _T("ce2ce.out.txt"), _T("\\z\\ce2ce.txt"));
    if (zr != ZR_OK) msg(_T("* Failed to zip ce2ce.txt"));
    zr = CloseZip(hz);
    if (zr != ZR_OK) msg(_T("* Failed to close ce2ce.zip"));
    if (p_abort) return 1;
    hz = OpenZip(_T("\\z\\ce2ce.zip"), 0);
    if (hz == 0) msg(_T("* Failed to open ce2ce.zip"));
    zr = SetUnzipBaseDir(hz, _T("\\z"));
    if (zr != ZR_OK) msg(_T("* Failed to set base dir for ce2ce.zip"));
    zr = GetZipItem(hz, -1, &ze);
    if (zr != ZR_OK) msg(_T("* Failed to get index of ce2ce.zip"));
    zr = GetZipItem(hz, 0, &ze);
    if (zr != ZR_OK) msg(_T("* Failed to GetItem(ce2ce.jpg)"));
    zr = UnzipItem(hz, 0, ze.name);
    if (zr != ZR_OK) msg(_T("* Failed to unzip ce2ce.out.jpg"));
    zr = GetZipItem(hz, 1, &ze);
    if (zr != ZR_OK) msg(_T("* Failed to GetItem(ce2ce.txt)"));
    zr = UnzipItem(hz, 1, ze.name);
    if (zr != ZR_OK) msg(_T("* Failed to unzip ce2ce.out.txt"));
    zr = CloseZip(hz);
    if (zr != ZR_OK) msg(_T("* Failed to close ce2ce.zip"));
    if (!fsame<msg>(_T("\\z\\ce2ce.jpg"), _T("\\z\\ce2ce.out.jpg")))
      msg(_T("* Failed to unzip all of ce2ce.jpg"));
    if (!fsame<msg>(_T("\\z\\ce2ce.txt"), _T("\\z\\ce2ce.out.txt")))
      msg(_T("* Failed to unzip all of ce2ce.txt"));
    if (p_abort) return 1;
  }

  // fixed bug: doesn't work with multiple levels of subfolder
  // fixed bug: EnsureDirectory,  bug: line 3940 of unzip, replace
  // _tcscat(cd,name); with _tcscat(cd,dir); otherwise root directories are
  // never created!
  msg(_T("folders - testing whether unzip to subfolders works"));
  {
    {
      constexpr TCHAR c[] = _T("Hello just a dummy file\r\n");
      handle_ptr<msg> hf{
          CreateFile(_T("\\z\\folders.txt"), GENERIC_WRITE, 0, 0, CREATE_ALWAYS,
                     FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_ARCHIVE |
                         FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_SEQUENTIAL_SCAN,
                     0)};
      if (!hf) msg(_T("* Failed to create \\z\\folders.txt"));

      if (!WriteFile(hf.get(), c, (DWORD)(_tcslen(c) * sizeof(TCHAR)), &writ,
                     0)) {
        msg(_T("* Failed to write \\z\\folders.txt"));
      }
    }

    hz = CreateZip(_T("\\z\\folders.zip"), 0);
    if (hz == 0) msg(_T("* Failed to create folders.zip"));
    zr = ZipAdd(hz, _T("folders.out.txt"), _T("\\z\\folders.txt"));
    if (zr != ZR_OK) msg(_T("* Failed to add folders.txt"));
    zr = ZipAdd(hz, _T("sub1\\folders.out.txt"), _T("\\z\\folders.txt"));
    if (zr != ZR_OK) msg(_T("* Failed to add sub1\\folders.txt"));
    zr = ZipAdd(hz, _T("sub1\\sub2\\folders.out.txt"), _T("\\z\\folders.txt"));
    if (zr != ZR_OK) msg(_T("* Failed to add sub1\\sub2\\folders.txt"));
    zr = ZipAdd(hz, _T("sub1\\sub2\\sub3\\sub4\\folders.out.txt"),
                _T("\\z\\folders.txt"));
    if (zr != ZR_OK)
      msg(_T("* Failed to add sub1\\sub2\\sub3\\sub4\\folders.txt"));
    zr = ZipAddFolder(hz, _T("sub1\\sub5\\sub6"));
    if (zr != ZR_OK) msg(_T("* Failed to add sub1\\sub5\\sub6"));
    zr = CloseZip(hz);
    if (zr != ZR_OK) msg(_T("* Failed to close folders.zip"));
    if (p_abort) return 1;
    hz = OpenZip(_T("\\z\\folders.zip"), 0);
    if (hz == 0) msg(_T("* Failed to open folders.zip"));
    zr = SetUnzipBaseDir(hz, _T("\\z"));
    if (zr != ZR_OK) msg(_T("* Failed to set base dir for folders.zip"));
    zr = GetZipItem(hz, -1, &ze);
    if (zr != ZR_OK) msg(_T("* Failed to get index of folders.zip"));
    int numitems = ze.index;
    for (int i = 0; i < numitems; i++) {
      zr = GetZipItem(hz, i, &ze);
      if (zr != ZR_OK) {
        wsprintf(m, _T("* Failed to get index %i of folders.zip"), i);
        msg(m);
      }
      zr = UnzipItem(hz, i, ze.name);
      if (zr != ZR_OK) {
        wsprintf(m, _T("* Failed to unzip item %i of folders.zip"), i);
        msg(m);
      }
      TCHAR absfn[MAX_PATH];
      wsprintf(absfn, _T("\\z\\%s"), ze.name);
      if (!IsDirectory(ze) && !fsame<msg>(_T("\\z\\folders.txt"), absfn)) {
        wsprintf(m, _T("* Failed to unzip all of item %i of folders.zip"), i);
        msg(m);
      }
    }
    zr = CloseZip(hz);
    if (zr != ZR_OK) msg(_T("* Failed to close folders.zip"));
    if (p_abort) return 1;
  }

  // fixed bug: it skips first few (8/12) bytes when extracting a big file
  // (27760609; 17838044 is okay)
  msg(_T("skip - testing whether it skips bytes when extracting big files... ")
      _T("(will take a while)"));
  {
    constexpr DWORD size = 27760609;

    std::vector<char> c;
    c.reserve(size);
    for (DWORD i = 0; i < size; i++) c.emplace_back((char)(rand() % 255));

    {
      handle_ptr<msg> hf{
          CreateFile(_T("\\z\\skip.dat"), GENERIC_WRITE, 0, 0, CREATE_ALWAYS,
                     FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_ARCHIVE |
                         FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_SEQUENTIAL_SCAN,
                     0)};
      if (!hf) msg(_T("* Failed to create \\z\\skip.dat"));

      if (!WriteFile(hf.get(), c.data(), size, &writ, 0)) {
        msg(_T("* Failed to write \\z\\skip.dat"));
      }
    }

    //
    hz = CreateZip(_T("\\z\\skip.zip"), 0);
    if (hz == 0) msg(_T("* Failed to create skip.zip"));
    zr = ZipAdd(hz, _T("skip.dat"), _T("\\z\\skip.dat"));
    if (zr != ZR_OK) msg(_T("* Failed to add skip.dat"));
    zr = CloseZip(hz);
    if (zr != ZR_OK) msg(_T("* Failed to close skip.zip"));
    if (p_abort) return 1;
    hz = OpenZip(_T("\\z\\skip.zip"), 0);
    if (hz == 0) msg(_T("* Failed to open skip.zip"));
    zr = UnzipItem(hz, 0, _T("\\z\\skip.out.dat"));
    if (zr != ZR_OK) msg(_T("* Failed to unzip skip.out.zip"));
    zr = CloseZip(hz);
    if (zr != ZR_OK) msg(_T("* Failed to close skip.zip"));
    if (!fsame<msg>(_T("\\z\\skip.dat"), _T("\\z\\skip.out.dat")))
      msg(_T("* Failed to unzip all of skip.zip"));
    if (p_abort) return 1;
  }

  // fixed bug: had problems with big files
  msg(_T("win - testing with zipping up the whole windows directory... (will ")
      _T("take a while)"));
  hz = CreateZip(_T("\\z\\win.big"), 0);
  if (hz == 0) msg(_T("* Failed to create win.big"));

  TCHAR systemdirpattern[MAX_PATH];
  if (!GetSystemDirectory(systemdirpattern, ARRAYSIZE(systemdirpattern))) {
    msg(_T("* Failed to get system directory"));
  }

  size_t systemdirpatternsize = _tcslen(systemdirpattern);
  // This path does not end with a backslash unless the system directory is the
  // root directory.
  //
  // See
  // https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-getsystemdirectorya
  if (systemdirpatternsize > 0 &&
      systemdirpattern[systemdirpatternsize - 1] != _T('\\')) {
    if (systemdirpatternsize < ARRAYSIZE(systemdirpattern) - 1) {
      systemdirpattern[systemdirpatternsize++] = _T('\\');
      systemdirpattern[systemdirpatternsize] = _T('\0');
    }
  }

  if (systemdirpatternsize >= ARRAYSIZE(systemdirpattern) - 1) {
    msg(_T("* Failed to build find files pattern for system directory"));
    systemdirpattern[ARRAYSIZE(systemdirpattern) - 3] = _T('\\');
    systemdirpattern[ARRAYSIZE(systemdirpattern) - 2] = _T('*');
    systemdirpattern[ARRAYSIZE(systemdirpattern) - 1] = _T('\0');
  } else {
    _tcscat(systemdirpattern, _T("*"));
    systemdirpattern[++systemdirpatternsize] = _T('\0');
  }

  LONGLONG tot = 0;
  constexpr DWORD max = 64 * 1024 * 1024;
  WIN32_FIND_DATA fdat;
  HANDLE hfind = FindFirstFile(systemdirpattern, &fdat);

  if (systemdirpatternsize >= ARRAYSIZE(systemdirpattern) - 1) {
    msg(_T("* Failed to build find file name format for system directory"));
    systemdirpattern[ARRAYSIZE(systemdirpattern) - 3] = _T('%');
    systemdirpattern[ARRAYSIZE(systemdirpattern) - 2] = _T('s');
    systemdirpattern[ARRAYSIZE(systemdirpattern) - 1] = _T('\0');
  } else {
    systemdirpattern[systemdirpatternsize - 1] = _T('%');
    systemdirpattern[systemdirpatternsize++] = _T('s');
    systemdirpattern[systemdirpatternsize++] = _T('\0');
  }

  for (BOOL gotmore = (hfind != INVALID_HANDLE_VALUE); gotmore;
       gotmore = FindNextFile(hfind, &fdat)) {
    if (fdat.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
    if (fdat.cFileName[0] == '$') continue;

    TCHAR src[MAX_PATH];
    _sntprintf(src, ARRAYSIZE(src), systemdirpattern, fdat.cFileName);

    TCHAR dst[MAX_PATH];
    _sntprintf(dst, ARRAYSIZE(dst), _T("out_%s"), fdat.cFileName);
    _sntprintf(m, ARRAYSIZE(m), _T(".%s (%lld%%)"), src, (tot * 100 / max));

    msg(m);

    LARGE_INTEGER size;
    {
      handle_ptr<msg> hf{CreateFile(
          src, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING,
          FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_SEQUENTIAL_SCAN, 0)};
      if (!hf) continue;

      if (!GetFileSizeEx(hf.get(), &size)) continue;
    }

    tot += size.QuadPart;

    zr = ZipAdd(hz, dst, src);
    if (zr != ZR_OK) {
      _sntprintf(m, ARRAYSIZE(m), _T("* Failed to add %s to win.big"), src);
      msg(m);
    }
    if (tot > max) break;
    if (p_abort) break;
  }
  if (hfind != INVALID_HANDLE_VALUE) FindClose(hfind);
  zr = CloseZip(hz);
  if (zr != ZR_OK) msg(_T("* Failed to close win.big"));
  if (p_abort) return 1;
  //
  hz = CreateZip(_T("\\z\\big.zip"), 0);
  if (hz == 0) msg(_T("* Failed to create big.zip"));
  zr = ZipAdd(hz, _T("win.out.big"), _T("\\z\\win.big"));
  if (zr != ZR_OK) msg(_T("* Failed to add win.big to big.zip"));
  zr = CloseZip(hz);
  if (zr != ZR_OK) msg(_T("* Failed to close big.zip"));
  if (p_abort) return 1;
  hz = OpenZip(_T("\\z\\big.zip"), 0);
  if (hz == 0) msg(_T("* Failed to open big.zip"));
  zr = UnzipItem(hz, 0, _T("\\z\\win.out.big"));
  if (zr != ZR_OK) msg(_T("* Failed to unzip win.big"));
  zr = CloseZip(hz);
  if (zr != ZR_OK) msg(_T("* Failed to close big.zip"));
  if (!fsame<msg>(_T("\\z\\win.big"), _T("\\z\\win.out.big")))
    msg(_T("* Failed to unzip all of win.big"));
  if (p_abort) return 1;

  if (any_errors) {
    msg(_T("Finished"));
    return 1;
  }

  msg(_T("Finished. No errors."));
  return 0;
}
