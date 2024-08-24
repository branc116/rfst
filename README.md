# Text rendering using stb_truetype

Program that renders it's own source code. Just testing what can be done with stb_truetype...

## Build & Run on windows

```powershell
clang -L .\external\lib -lraylibdll main.c
cp external/lib/raylib.dll .
a.exe
```

## Build & Run on linux

```sh
gcc -O3 main.c -lm -lraylib && ./a.out
```

## Usage


