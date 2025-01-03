// ZIP functions -- for creating zip files
//
// This file is a repackaged form of the Info-Zip source code available at
// www.info-zip.org.  The original copyright notice may be found in XZip.cpp.
// The repackaging was done by Lucian Wischik to simplify and extend its use in
// Windows/C++.  Also to add encryption and unicode.
//
// e.g.
//
// (1) Traditional use, creating a zipfile from existing files:
//
//   HZIP hz = CreateZip("c:\\simple1.zip",0);
//   ZipAdd(hz,"znsimple.bmp", "c:\\simple.bmp");
//   ZipAdd(hz,"znsimple.txt", "c:\\simple.txt");
//   CloseZip(hz);
//
// (2) Memory use, creating an auto-allocated mem-based zip file from various
// sources:
//
//   HZIP hz = CreateZip(0,100000, 0);
//   // adding a conventional file...
//   ZipAdd(hz,"src1.txt",  "c:\\src1.txt");
//   // adding something from memory...
//   char buf[1000]; for (int i=0; i<1000; i++) buf[i]=(char)(i&0x7F);
//   ZipAdd(hz,"file.dat",  buf,1000);
//   // adding something from a pipe...
//   HANDLE hread,hwrite; CreatePipe(&hread,&hwrite,NULL,0);
//   HANDLE hthread = CreateThread(0,0,ThreadFunc,(void*)hwrite,0,0);
//   ZipAdd(hz,"unz3.dat",  hread,1000);  // the '1000' is optional.
//   WaitForSingleObject(hthread,INFINITE);
//   CloseHandle(hthread); CloseHandle(hread);
//   ... meanwhile DWORD WINAPI ThreadFunc(void *dat)
//                 { HANDLE hwrite = (HANDLE)dat;
//                   char buf[1000]={17};
//                   DWORD writ; WriteFile(hwrite,buf,1000,&writ,NULL);
//                   CloseHandle(hwrite);
//                   return 0;
//                 }
//   // and now that the zip is created, let's do something with it:
//   void *zbuf; unsigned long zlen; ZipGetMemory(hz,&zbuf,&zlen);
//   HANDLE hfz =
//   CreateFile("test2.zip",GENERIC_WRITE,0,0,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);
//   DWORD writ; WriteFile(hfz,zbuf,zlen,&writ,NULL);
//   CloseHandle(hfz);
//   CloseZip(hz);
//
// (3) Handle use, for file handles and pipes
//   HANDLE hzread,hzwrite; CreatePipe(&hzread,&hzwrite,0,0);
//   HANDLE hthread = CreateThread(0,0,ZipReceiverThread,(void*)hzread,0,0);
//   HZIP hz = CreateZipHandle(hzwrite,0);
//   // ... add to it
//   CloseZip(hz);
//   CloseHandle(hzwrite);
//   WaitForSingleObject(hthread,INFINITE);
//   CloseHandle(hthread);
//   ... meanwhile DWORD WINAPI ZipReceiverThread(void *dat)
//                 { HANDLE hread = (HANDLE)dat;
//                   char buf[1000];
//                   while (true)
//                   { DWORD red; ReadFile(hread,buf,1000,&red,NULL);
//                     // ... and do something with this zip data we're
//                     receiving if (red==0) break;
//                   }
//                   CloseHandle(hread);
//                   return 0;
//                 }

#ifndef ZIP_UTILS_XZIP_H_
#define ZIP_UTILS_XZIP_H_

#ifdef ZIP_STD
#include <time.h>
#define DECLARE_HANDLE(name) \
  struct name##__ {          \
    int unused;              \
  };                         \
  typedef struct name##__ *name
#ifndef MAX_PATH
#define MAX_PATH 1024
#endif
typedef unsigned long DWORD;
typedef char TCHAR;
typedef FILE *HANDLE;
#else
#define MAX_PATH 260
// An HZIP identifies a zip file that is being created.
using HZIP = struct HZIP__ *;
typedef unsigned long DWORD;
typedef char TCHAR;
typedef void *HANDLE;
#endif

// return codes from any of the zip functions.  Listed later.
enum ZRESULT : DWORD {
  ZR_OK = 0x00000000,
  // nb. the pseudo-code zr-recent is never returned but can be passed to
  // FormatZipMessage.
  ZR_RECENT = 0x00000001,

  // The following come from general system stuff (e.g. files not openable)

  ZR_GENMASK = 0x0000FF00,
  // couldn't duplicate the handle
  ZR_NODUPH = 0x00000100,
  // couldn't create/open the file
  ZR_NOFILE = 0x00000200,
  // failed to allocate some resource
  ZR_NOALLOC = 0x00000300,
  // a general error writing to the file
  ZR_WRITE = 0x00000400,
  // couldn't find that file in the zip
  ZR_NOTFOUND = 0x00000500,
  // there's still more data to be unzipped
  ZR_MORE = 0x00000600,
  // the zipfile is corrupt or not a zipfile
  ZR_CORRUPT = 0x00000700,
  // a general error reading the file
  ZR_READ = 0x00000800,

  // The following come from mistakes on the part of the caller

  ZR_CALLERMASK = 0x00FF0000,
  // general mistake with the arguments
  ZR_ARGS = 0x00010000,
  // tried to ZipGetMemory, but that only works on mmap zipfiles, which yours
  // wasn't
  ZR_NOTMMAP = 0x00020000,
  // the memory size is too small
  ZR_MEMSIZE = 0x00030000,
  // the thing was already failed when you called this function
  ZR_FAILED = 0x00040000,
  // the zip creation has already been closed
  ZR_ENDED = 0x00050000,
  // the indicated input file size turned out mistaken
  ZR_MISSIZE = 0x00060000,
  // the file had already been partially unzipped
  ZR_PARTIALUNZ = 0x00070000,
  // tried to mix creating/opening a zip
  ZR_ZMODE = 0x00080000,

  // The following come from bugs within the zip library itself

  ZR_BUGMASK = 0xFF000000,
  // initialization didn't work
  ZR_NOTINITED = 0x01000000,
  // trying to seek in an unseekable file
  ZR_SEEK = 0x02000000,
  // changed its mind on storage, but not allowed
  ZR_NOCHANGE = 0x04000000,
  // an internal error in the de/inflation code
  ZR_FLATE = 0x05000000
};

// CreateZip - call this to start the creation of a zip file.
//
// As the zip is being created, it will be stored somewhere:
// to a pipe:              CreateZipHandle(hpipe_write);
// in a file (by handle):  CreateZipHandle(hfile);
// in a file (by name):    CreateZip("c:\\test.zip");
// in memory:              CreateZip(buf, len);
// or in pagefile memory:  CreateZip(0, len);
//
// The final case stores it in memory backed by the system paging file, where
// the zip may not exceed len bytes.  This is a bit friendlier than allocating
// memory with new[]: it won't lead to fragmentation, and the memory won't be
// touched unless needed.  That means you can give very large estimates of the
// maximum-size without too much worry.  As for the password, it lets you
// encrypt every file in the archive.  (This api doesn't support per-file
// encryption.)
//
// NOTE: because pipes don't allow random access, the structure of a zipfile
// created into a pipe is slightly different from that created into a file
// or memory.  In particular, the compressed-size of the item cannot be stored
// in the zipfile until after the item itself.  (Also, for an item added itself
// via a pipe, the uncompressed-size might not either be known until after.)
//
// This is not normally a problem.  But if you try to unzip via a pipe as well,
// then the unzipper will not know these things about the item until after it
// has been unzipped.  Therefore: for unzippers which don't just write each item
// to disk or to a pipe, but instead pre-allocate memory space into which to
// unzip them, then either you have to create the zip not to a pipe, or you have
// to add items not from a pipe, or at least when adding items from a pipe you
// have to specify the length.
//
// NOTE: for windows-ce, you cannot close the handle until after CloseZip.  But
// for real windows, the zip makes its own copy of your handle, so you can close
// yours anytime.
HZIP CreateZip(const TCHAR *fn, const char *password);
HZIP CreateZip(void *buf, unsigned int len, const char *password);
HZIP CreateZipHandle(HANDLE h, const char *password);

// ZipAdd - call this for each file to be added to the zip.
//
// dstzn is the name that the file will be stored as in the zip file.
//
// The file to be added to the zip can come
// from a pipe:  ZipAddHandle(hz,"file.dat", hpipe_read);
// from a file:  ZipAddHandle(hz,"file.dat", hfile);
// from a filen: ZipAdd(hz,"file.dat", "c:\\docs\\origfile.dat");
// from memory:  ZipAdd(hz,"subdir\\file.dat", buf,len);
// (folder):     ZipAddFolder(hz,"subdir");
//
// NOTE: if adding an item from a pipe, and if also creating the zip file itself
// to a pipe, then you might wish to pass a non-zero length to the ZipAddHandle
// function.  This will let the zipfile store the item's size ahead of the
// compressed item itself, which in turn makes it easier when unzipping the
// zipfile from a pipe.
ZRESULT ZipAdd(HZIP hz, const TCHAR *dstzn, const TCHAR *fn);
ZRESULT ZipAdd(HZIP hz, const TCHAR *dstzn, void *src, unsigned int len);
ZRESULT ZipAddHandle(HZIP hz, const TCHAR *dstzn, HANDLE h);
ZRESULT ZipAddHandle(HZIP hz, const TCHAR *dstzn, HANDLE h, unsigned int len);
ZRESULT ZipAddFolder(HZIP hz, const TCHAR *dstzn);

// ZipGetMemory - If the zip was created in memory, via ZipCreate(0,len), then
// this function will return information about that memory block.  Buf will
// receive a pointer to its start, and len its length.
//
// NOTE: you can't add any more after calling this.
ZRESULT ZipGetMemory(HZIP hz, void **buf, unsigned long *len);

// CloseZip - the zip handle must be closed with this function.
ZRESULT CloseZip(HZIP hz);

// FormatZipMessage - given an error code, formats it as a string.
//
// It returns the length of the error message.  If buf/len points to a real
// buffer, then it also writes as much as possible into there.
unsigned int FormatZipMessage(ZRESULT code, TCHAR *buf, unsigned int len);

// Now we indulge in a little skullduggery so that the code works whether the
// user has included just zip or both zip and unzip.
//
// Idea: if header files for both zip and unzip are present, then presumably the
// cpp files for zip and unzip are both present, so we will call one or the
// other of them based on a dynamic choice.  If the header file for only one is
// present, then we will bind to that particular one.
ZRESULT CloseZipZ(HZIP hz);
unsigned int FormatZipMessageZ(ZRESULT code, char *buf, unsigned int len);
bool IsZipHandleZ(HZIP hz);

#ifdef ZIP_UTILS_XUNZIP_H_
#undef CloseZip
#define CloseZip(hz) (IsZipHandleZ(hz) ? CloseZipZ(hz) : CloseZipU(hz))
#else
#define CloseZip CloseZipZ
#define FormatZipMessage FormatZipMessageZ
#endif

#endif  // ZIP_UTILS_XZIP_H_
