#include <cstdio>
#include <memory>
#include <new>

#include "../../XUnzip.h"
#include "../../XZip.h"
#include "../test_utils.h"

#include <direct.h>

using namespace zu_utils::examples;

int main() {
  msg("Zip and unzip in portable stdlib-only mode");

  using zip_ptr = std::unique_ptr<HZIP__, ZipDeleter<msg>>;

  {
    zip_ptr hz{CreateZip("std1.zip", nullptr)};
    if (!hz) msg("* Failed to create std1.zip");

    ZRESULT rc = ZipAdd(hz.get(), "znsimple.jpg", "std_sample.jpg");
    if (rc != ZR_OK) msg("* Failed to add znsimple.jpg to zip");

    rc = ZipAdd(hz.get(), "znsimple.txt", "std_sample.txt");
    if (rc != ZR_OK) msg("* Failed to add znsimple.txt to zip");
  }

  {
    zip_ptr hz{OpenZip("std1.zip", nullptr)};
    if (!hz) msg("* Failed to open std1.zip");

    ZIPENTRY ze;
    ZRESULT rc = GetZipItem(hz.get(), -1, &ze);
    if (rc != ZR_OK) msg("* Failed to get root zip item");

    int numitems = ze.index;
    for (int zi = 0; zi < numitems; zi++) {
      rc = GetZipItem(hz.get(), zi, &ze);
      if (rc != ZR_OK) msg("* Failed to get N zip item");

      rc = UnzipItem(hz.get(), zi, ze.name);
      if (rc != ZR_OK) msg("* Failed to unzip N zip item");
    }
  }

  if (!fsame<msg>("znsimple.jpg", "std_sample.jpg")) {
    msg("* Failed to unzip std_sample.jpg");
  }

  if (!fsame<msg>("znsimple.txt", "std_sample.txt")) {
    msg("* Failed to unzip std_sample.txt");
  }

  if (any_errors) {
    msg("Finished");
    return 1;
  }

  msg("Finished. No errors.");
  return 0;
}
