\mingw\bin\windres -o testres.o test.rc
\mingw\bin\c++ -o test.exe test.cpp testres.o ..\..\XZip.cpp ..\..\XUnzip.cpp
