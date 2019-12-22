rem Ensure Cygwin executables like sh.exe are not in PATH
set PATH=C:\Windows\system32;C:\Windows;C:\mingw-w64\x86_64-4.9.1-posix-seh-rt_v3-rev1\mingw64\bin;C:\mongo-c-driver\bin

echo CONFIGURE_FLAGS %CONFIGURE_FLAGS%

set CMAKE=C:\cmake\bin\cmake
set CMAKE_MAKE_PROGRAM=C:\mingw-w64\x86_64-4.9.1-posix-seh-rt_v3-rev1\mingw64\bin\mingw32-make.exe
set CC=C:\mingw-w64\x86_64-4.9.1-posix-seh-rt_v3-rev1\mingw64\bin\gcc.exe

cd src\libbson
%CMAKE% -G "MinGW Makefiles" -DCMAKE_MAKE_PROGRAM=%CMAKE_MAKE_PROGRAM% -DCMAKE_INSTALL_PREFIX=C:\mongo-c-driver %CONFIGURE_FLAGS%

%CMAKE_MAKE_PROGRAM%
%CMAKE_MAKE_PROGRAM% install

cd ..\..
%CMAKE% -G "MinGW Makefiles" -DCMAKE_MAKE_PROGRAM=%CMAKE_MAKE_PROGRAM% -DBSON_ROOT_DIR=C:\mongo-c-driver -DCMAKE_INSTALL_PREFIX=C:\mongo-c-driver %CONFIGURE_FLAGS%

%CMAKE_MAKE_PROGRAM%
%CMAKE_MAKE_PROGRAM% install

set MONGOC_TEST_SKIP_LIVE=on
test-libmongoc.exe --no-fork -d -F test-results.json
