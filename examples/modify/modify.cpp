// This program shows how to modify an existing zipfile -- add to it, remove
// from it

#include <iostream>

#include "../test_utils.h"
#include "XUnzip.h"
#include "XZip.h"

#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif

using namespace zu_utils::examples;

namespace {

using file_ptr = std::unique_ptr<FILE, FileDeleter<msg>>;
using zip_ptr = std::unique_ptr<HZIP__, ZipDeleter<msg>>;

ZRESULT AddFileToZip(const TCHAR *zipfn, const TCHAR *zename,
                     const TCHAR *zefn) {
  if (!IsFileExist(zipfn) || (zefn != nullptr && !IsFileExist(zefn))) {
    return ZR_NOFILE;
  }

  const long size0{fsize(file_ptr{fopen(zipfn, "rb")})};
  if (size0 < 0) return ZR_NOFILE;

  long size1 = 0;
  if (zefn) {
    size1 = fsize(file_ptr{fopen(zefn, "rb")});

    if (size1 < 0) return ZR_NOFILE;
  }

  DWORD size = static_cast<DWORD>(size0) + static_cast<DWORD>(size1);

  size *= 2;  // just to be on the safe side.

  std::unique_ptr<char[]> buf{nullptr};

  zip_ptr hzdst{CreateZip(0, size, 0)};
  if (!hzdst) return ZR_WRITE;

  char *hzdstcontent = nullptr;

  {
    zip_ptr hzsrc{OpenZip(zipfn, 0)};
    if (!hzsrc) return ZR_READ;

    ZIPENTRY ze;
    ZRESULT zr = GetZipItem(hzsrc.get(), -1, &ze);
    if (zr != ZR_OK) return zr;

    int numitems = ze.index;
    // hzdst is created in the system pagefile
    // Now go through the old zip, unzipping each item into a memory buffer, and
    // adding it to the new one
    unsigned int bufsize = 0;  // we'll unzip each item into this memory buffer

    for (int i = 0; i < numitems; i++) {
      zr = GetZipItem(hzsrc.get(), i, &ze);
      if (zr != ZR_OK) return zr;

      // don't copy over the old version of the file we're changing
      if (strcmp(ze.name, zename) == 0) continue;

      if (IsDirectory(ze)) {
        zr = ZipAddFolder(hzdst.get(), ze.name);
        if (zr != ZR_OK) return zr;

        continue;
      }

      if (ze.unc_size > (long)bufsize) {
        buf.reset(nullptr);

        bufsize = ze.unc_size * 2;
        buf = std::make_unique<char[]>(bufsize);
      }

      zr = UnzipItem(hzsrc.get(), i, buf.get(), ze.unc_size);
      if (zr != ZR_OK) return zr;

      zr = ZipAdd(hzdst.get(), ze.name, buf.get(), ze.unc_size);
      if (zr != ZR_OK) return zr;
    }

    // Now add the new file
    if (zefn != 0) {
      zr = ZipAdd(hzdst.get(), zename, zefn);
      if (zr != ZR_OK) return zr;
    }

    //
    // The new file has been put into pagefile memory. Let's store it to disk,
    // overwriting the original zip
    zr = ZipGetMemory(hzdst.get(), reinterpret_cast<void **>(&hzdstcontent),
                      &size);
    if (zr != ZR_OK) return zr;
  }

  file_ptr hf{fopen(zipfn, "wb")};
  if (!hf) return ZR_WRITE;

  return fwrite(hzdstcontent, 1, size, hf.get()) == size ? ZR_OK : ZR_WRITE;
}

// AddFileToZip: adds a file to a zip, possibly replacing what was there before
// zipfn ="c:\\archive.zip"             (the fn of the zip file)
// zefn  ="c:\\my documents\\file.txt"  (the fn of the file to be added)
// zename="file.txt"                    (the name that zefn will take inside the
// zip)
//
// If zefn is empty, we just delete zename from the zip archive.  The way it
// works is that we create a temporary zipfile, and copy the original contents
// into the new one (with the appropriate addition or substitution) and then
// remove the old one and rename the new one.  NB.  we are case-insensitive.
ZRESULT RemoveFileFromZip(const TCHAR *zipfn, const TCHAR *zename) {
  return AddFileToZip(zipfn, zename, nullptr);
}

}  // namespace

// AddFileToZip:
// eg. zipfn="c:\\archive.zip", zefn="c:\\docs\\file.txt", zename="file.txt"
//
// If the zipfile already contained something called zename (case-insensitive),
// it is removed.  These two functions are defined below.

int main() {
  msg("This program shows how to modify an existing zipfile -- add to it, "
      "remove from it.");

  // First we'll create some small files
  if (zumkdir("\\z") == -1 && errno != EEXIST) {
    msg("* Failed to create directory \\z");
  }

  using file_ptr = std::unique_ptr<FILE, FileDeleter<msg>>;

  const char *s = "a contents\r\n";

  {
    file_ptr f{fopen("\\z\\a.txt", "wt")};
    if (fputs(s, f.get()) < 0) msg("* Failed to put string to \\z\\a.txt");
  }

  s = "b stuff\r\n";

  {
    file_ptr f{fopen("\\z\\b.txt", "wt")};
    if (fputs(s, f.get()) < 0) msg("* Failed to put string to \\z\\b.txt");
  }

  s = "c something\r\n";

  {
    file_ptr f{fopen("\\z\\c.txt", "wt")};
    if (fputs(s, f.get()) < 0) msg("* Failed to put string to \\z\\c.txt");
  }

  s = "c fresh\r\n";

  {
    file_ptr f{fopen("\\z\\c2.txt", "wt")};
    if (fputs(s, f.get()) < 0) msg("* Failed to put string to \\z\\c2.txt");
  }

  s = "d up\r\n";

  {
    file_ptr f{fopen("\\z\\d.txt", "wt")};
    if (fputs(s, f.get()) < 0) msg("* Failed to put string to \\z\\d.txt");
  }

  // and create a zip file to be working with
  msg("Creating '\\z\\modify.zip' with zna.txt, znb.txt, znc.txt and a "
      "folder...");

  {
    zip_ptr hz{CreateZip("\\z\\modify.zip", 0)};
    if (!hz) msg("* Failed to create zip \\z\\modify.zip");

    ZRESULT rc = ZipAdd(hz.get(), "zna.txt", "\\z\\a.txt");
    if (rc != ZR_OK) msg("* Failed to add zna.txt to zip");

    rc = ZipAdd(hz.get(), "znb.txt", "\\z\\b.txt");
    if (rc != ZR_OK) msg("* Failed to add znb.txt to zip");

    rc = ZipAdd(hz.get(), "znc.txt", "\\z\\c.txt");
    if (rc != ZR_OK) msg("* Failed to add znc.txt to zip");

    rc = ZipAddFolder(hz.get(), "znsub");
    if (rc != ZR_OK) msg("* Failed to add folder znsub to zip");
  }

  msg("Adding znsub\\znd.txt to the zip file...");
  ZRESULT rc = AddFileToZip("\\z\\modify.zip", "znsub\\znd.txt", "\\z\\d.txt");
  if (rc != ZR_OK) msg("* Failed to add znsub\\znd.txt to the zip file");

  msg("Removing znb.txt from the zip file...");
  rc = RemoveFileFromZip("\\z\\modify.zip", "znb.txt");
  if (rc != ZR_OK) msg("* Failed to remove znb.txt from the zip file");

  msg("Updating znc.txt in the zip file...");
  rc = AddFileToZip("\\z\\modify.zip", "znc.txt", "\\z\\c2.txt");
  if (rc != ZR_OK) msg("* Failed to update znc.txt in the zip file");

  {
    msg("Checking unzipped contents");

    zip_ptr hz{OpenZip("\\z\\modify.zip", 0)};

    rc = SetUnzipBaseDir(hz.get(), "\\z");
    if (rc != ZR_OK) {
      msg("* Failed to set base unzip dir for \\z\\modify.zip");
    }

    ZIPENTRY ze;
    rc = GetZipItem(hz.get(), -1, &ze);
    if (rc != ZR_OK) {
      msg("* Failed to get items count for zip \\z\\modify.zip");
    }

    if (chdir("\\z") == -1) {
      msg("* Failed to chdir to \\z");
    }

    int numitems = ze.index;
    if (numitems != 4) msg("* Failed to get 4 items from \\z\\modify.zip");

    for (int zi = 0; zi < numitems; zi++) {
      rc = GetZipItem(hz.get(), zi, &ze);
      if (rc != ZR_OK) {
        msg("* Failed to get N item for zip \\z\\modify.zip");
      }

      rc = UnzipItem(hz.get(), zi, ze.name);
      if (rc != ZR_OK) {
        msg("* Failed to unzip N item for zip \\z\\modify.zip");
      }

      if (strcmp(ze.name, "zna.txt") == 0) {
        if (!fsame<msg>(ze.name, "a.txt")) {
          msg("* Failed to unzip zna.txt from zip \\z\\modify.zip");
        }
      } else if (strcmp(ze.name, "znc.txt") == 0) {
        if (!fsame<msg>(ze.name, "c2.txt")) {
          msg("* Failed to unzip znc.txt from zip \\z\\modify.zip");
        }
      } else if (strcmp(ze.name, "znsub/") == 0) {
        // do nothing for dir.
      } else if (strcmp(ze.name, "znsub/znd.txt") == 0) {
        if (!fsame<msg>(ze.name, "d.txt")) {
          msg("* Failed to unzip znsub/znd.txt from zip \\z\\modify.zip");
        }
      } else {
        msg("* Unexpected file in zip \\z\\modify.zip");
      }
    }

    if (rc == ZR_OK) msg("Unzipped contents from \\z\\modify.zip");
  }

  if (any_errors) {
    msg("Finished");
    return 1;
  }

  msg("Finished. No errors.");
  return 0;
}
