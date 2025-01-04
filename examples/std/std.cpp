#include <cstdio>
#include <memory>

#include "../../XUnzip.h"
#include "../../XZip.h"

namespace {

bool fsame(const char *fn0, const char *fn1) {
  FILE *hf0 = fopen(fn0, "rb");
  FILE *hf1 = fopen(fn1, "rb");
  if (hf0 == nullptr || hf1 == nullptr) {
    if (hf0 != nullptr) fclose(hf0);
    if (hf1 != nullptr) fclose(hf1);
    return false;
  }

  fseek(hf0, 0, SEEK_END);
  long size0 = ftell(hf0);
  fseek(hf0, 0, SEEK_SET);

  fseek(hf1, 0, SEEK_END);
  long size1 = ftell(hf1);
  fseek(hf1, 0, SEEK_SET);

  if (size0 != size1) {
    fclose(hf0);
    fclose(hf1);
    return false;
  }
  long size = size0;
  //
  constexpr int readsize = 65535;
  std::unique_ptr<char[]> buf0 = std::make_unique<char[]>(readsize);
  std::unique_ptr<char[]> buf1 = std::make_unique<char[]>(readsize);
  long done = 0;
  while (done < size) {
    long left = size - done;

    if (left > readsize) left = readsize;

    size_t read = fread(buf0.get(), left, 1, hf0);
    if (read != 1) {
      break;
    }

    read = fread(buf1.get(), left, 1, hf1);
    if (read != 1) {
      break;
    }

    if (memcmp(buf0.get(), buf1.get(), read) != 0) break;

    done += left;
  }

  fclose(hf0);
  fclose(hf1);
  return (done == size);
}

}  // namespace

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
  } else {
    msg("Finished. No errors.");
    return 0;
  }
}
