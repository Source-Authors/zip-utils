#include <cstdio>
#include <memory>
#include <new>

#include "../../XUnzip.h"
#include "../../XZip.h"

namespace {

struct FileDeleter {
  void operator()(FILE *f) const noexcept { fclose(f); }
};

long fsize(const std::unique_ptr<FILE, FileDeleter> &f) noexcept {
  if (fseek(f.get(), 0, SEEK_END) == -1) return -1;

  const long size = ftell(f.get());

  if (fseek(f.get(), 0, SEEK_SET) == -1) return -1;

  return size;
}

bool fsame(const char *fn0, const char *fn1) noexcept {
  using file_ptr = std::unique_ptr<FILE, FileDeleter>;

  file_ptr hf0{fopen(fn0, "rb")};
  file_ptr hf1{fopen(fn1, "rb")};

  if (!hf0 || !hf1) return false;

  const long size0 = fsize(hf0);
  if (size0 == -1) return false;

  const long size1 = fsize(hf1);
  if (size1 == -1) return false;

  if (size0 != size1) return false;

  long size = size0;
  //
  constexpr int readsize = 65535;
  std::unique_ptr<char[]> buf0{new (std::nothrow) char[readsize]};
  std::unique_ptr<char[]> buf1{new (std::nothrow) char[readsize]};

  if (!buf0 || !buf1) return false;

  long done = 0;
  while (done < size) {
    long left = size - done;

    if (left > readsize) left = readsize;

    size_t read = fread(buf0.get(), left, 1, hf0.get());
    if (read != 1) {
      break;
    }

    read = fread(buf1.get(), left, 1, hf1.get());
    if (read != 1) {
      break;
    }

    if (memcmp(buf0.get(), buf1.get(), read) != 0) break;

    done += left;
  }

  return (done == size);
}

bool any_errors = false;
bool p_abort = false;

void msg(const TCHAR *s) {
  if (s[0] == '*') any_errors = true;

#ifdef UNDER_CE
  int res = IDOK;
  if (s[0] == '*')
    res = MessageBox(0, s, _T("Zip error"), MB_ICONERROR | MB_OKCANCEL);
  else if (s[0] == '.')
    MessageBeep(0);
  else
    MessageBox(0, s, _T("Zip test"), MB_OKCANCEL);
  if (res == IDCANCEL) p_abort = true;
#else

  if (!any_errors)
    printf("%s\n", s);
  else
    fprintf(stderr, "%s\n", s);
#endif
}

}  // namespace

int main() {
  HZIP hz = CreateZip("std1.zip", nullptr);
  if (!hz) msg("* Failed to create std1.zip");

  ZRESULT rc = ZipAdd(hz, "znsimple.jpg", "std_sample.jpg");
  if (rc != ZR_OK) msg("* Failed to add znsimple.jpg to zip");

  rc = ZipAdd(hz, "znsimple.txt", "std_sample.txt");
  if (rc != ZR_OK) msg("* Failed to add znsimple.txt to zip");

  rc = CloseZip(hz);
  if (rc != ZR_OK) msg("* Failed to close std1.zip");

  hz = OpenZip("std1.zip", nullptr);
  if (!hz) msg("* Failed to open std1.zip");

  ZIPENTRY ze;
  rc = GetZipItem(hz, -1, &ze);
  if (rc != ZR_OK) msg("* Failed to get root zip item");

  int numitems = ze.index;
  for (int zi = 0; zi < numitems; zi++) {
    rc = GetZipItem(hz, zi, &ze);
    if (rc != ZR_OK) msg("* Failed to get N zip item");

    rc = UnzipItem(hz, zi, ze.name);
    if (rc != ZR_OK) msg("* Failed to unzip N zip item");
  }

  rc = CloseZip(hz);
  if (rc != ZR_OK) msg("* Failed to close std1.zip");

  if (!fsame("znsimple.jpg", "std_sample.jpg")) {
    msg("* Failed to unzip std_sample.jpg");
  }

  if (!fsame("znsimple.txt", "std_sample.txt")) {
    msg("* Failed to unzip std_sample.txt");
  }

  if (any_errors) {
    msg("Finished");
    return 1;
  }

  msg("Finished. No errors.");
  return 0;
}
