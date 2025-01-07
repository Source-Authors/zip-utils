#ifndef ZU_UTILS_EXAMPLES_TEST_UTILS_H_
#define ZU_UTILS_EXAMPLES_TEST_UTILS_H_

#include <sys/stat.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <new>

#include "../XUnzip.h"
#include "../XZip.h"

#ifndef ZIP_STD
#include <Windows.h>
#endif

#if defined(_MSC_VER) || defined(__BORLANDC__) || defined(__MINGW32__)
#include <direct.h>
#define zumkdir(t) (mkdir(t))
#else
#include <unistd.h>
#define zumkdir(t) (mkdir(t, 0755))
#endif

namespace zu_utils::examples {

bool any_errors = false;
bool p_abort = false;

void msg(const char *s) {
  if (s[0] == '*') any_errors = true;

  if (!any_errors)
    std::cout << s << '\n';
  else
    std::cerr << s << '\n';

#ifndef ZIP_STD
  OutputDebugString(s);
  OutputDebugString("\n");
#endif
}

constexpr bool IsDirectory(const ZIPENTRY &ze) noexcept {
#ifndef ZIP_STD
  return ze.attr & FILE_ATTRIBUTE_DIRECTORY;
#else
  return ze.attr & S_IFDIR;
#endif
}

template <void msg(const char *s)>
struct FileDeleter {
  void operator()(FILE *f) const noexcept {
    if (fclose(f)) {
      msg("* Failed to close file");
      std::exit(1);
    }
  }
};

template <void msg(const char *s)>
struct ZipDeleter {
  void operator()(HZIP__ *f) const noexcept {
    const ZRESULT rc{CloseZip(f)};
    if (rc != ZR_OK) {
      msg("* Failed to close zip");
      std::exit(1);
    }
  }
};

#ifndef ZIP_STD
template <void msg(const char *s)>
class handle_ptr {
 public:
  handle_ptr() noexcept : handle_ptr{INVALID_HANDLE_VALUE} {}
  explicit handle_ptr(HANDLE h) noexcept : h_{h} {}

  handle_ptr(const handle_ptr &) = delete;
  handle_ptr &operator=(const handle_ptr &) = delete;

  ~handle_ptr() noexcept { reset(); }

  [[nodiscard]] HANDLE get() const noexcept { return h_; }

  [[nodiscard]] explicit operator bool() const noexcept {
    return get() != INVALID_HANDLE_VALUE;
  }

  [[nodiscard]] HANDLE *operator&() noexcept {
    reset();
    return &h_;
  }

 private:
  HANDLE h_;

  void reset() noexcept {
    if (*this) {
      if (!CloseHandle(h_)) {
        DWORD rc = ::GetLastError();
        msg("* Failed to close handle");
        exit(rc);
      }
    }
  }
};
#endif

template <void msg(const char *s)>
long fsize(const std::unique_ptr<FILE, FileDeleter<msg>> &f) noexcept {
  if (fseek(f.get(), 0, SEEK_END) == -1) return -1;

  const long size = ftell(f.get());

  if (fseek(f.get(), 0, SEEK_SET) == -1) return -1;

  return size;
}

bool IsFileExist(const TCHAR *fn) noexcept {
  using file_ptr = std::unique_ptr<FILE, FileDeleter<msg>>;

  file_ptr f{fopen(fn, "rb")};

  struct stat st;
  return fstat(fileno(f.get()), &st) != -1;
}

template <void msg(const char *s)>
bool fsame(const char *fn0, const char *fn1) noexcept {
  using file_ptr = std::unique_ptr<FILE, FileDeleter<msg>>;

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

}  // namespace zu_utils::examples

#endif  // ZU_UTILS_EXAMPLES_TEST_UTILS_H_
