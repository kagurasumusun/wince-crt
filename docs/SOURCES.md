# 参照資料 / Sources

本実装はクリーンルーム原則に従い、以下に挙げる公開された一次資料・仕様書のみを
根拠としている。特定の実装からコードをコピー・流用・改変していない。

## 一次資料・仕様

1. **Microsoft 公式ドキュメント (Windows CE 5.0/6.0/7.0)**
   - Microsoft C Run-time Library for Windows CE
     - https://learn.microsoft.com/en-us/previous-versions/windows/embedded/ms860390(v=msdn.10) (strcpy)
     - https://learn.microsoft.com/en-us/previous-versions/windows/embedded/ms860450(v=msdn.10) (strncpy)
     - https://learn.microsoft.com/en-us/previous-versions/windows/embedded/ms859665(v=msdn.10) (malloc)
     - https://learn.microsoft.com/en-us/previous-versions/windows/embedded/ms861162(v=msdn.10) (_strdup)
     - https://learn.microsoft.com/en-us/previous-versions/windows/embedded/ms861398(v=msdn.10) (vfprintf)
     - その他 Win32/CE API ドキュメント (coredll.dll のエクスポートリスト含む)
   - CRT 初期化の動作 (WinMainCRTStartup, _DllMainCRTStartup の仕様)
   - PE/COFF フォーマット: https://github.com/MicrosoftDocs/win32/blob/docs/desktop-src/Debug/pe-format.md

2. **ARM Architecture Procedure Call Standard (AAPCS / APCS)**
   - ARM IHI 0042E (AAPCS32):
     https://github.com/ARM-software/abi-aa/blob/main/aapcs32/aapcs32.rst
   - ARM DUI 0472 (Basic data types in ARM C and C++)
   - 旧 APCS (Windows CE で使用されるいわゆる Old ABI / -mapcs-32)
     スタックフレームのレイアウトと setjmp/longjmp のレジスタ保存ルール

3. **ISO/IEC 9899:1990 (C89), 9899:1999 (C99)**
   - 標準 C ライブラリ関数のシグネチャ・仕様
   - setjmp.h, stdio.h, stdlib.h, string.h, ctype.h, math.h, time.h, errno.h 等

4. **LLVM/Clang ドキュメント**
   - Cross-compilation with Clang: https://clang.llvm.org/docs/CrossCompilation.html
   - Target triple 形式 (arch-vendor-os-env)
   - __builtin_* 組込み関数の一覧
   - Compiler-rt が提供するヘルパーの一覧

5. **PE/COFF 仕様**
   - Microsoft PE Format ドキュメント
   - IMAGE_SUBSYSTEM_WINDOWS_CE_GUI = 9
   - DLL エントリポイントのシグネチャ (HANDLE hDll, DWORD reason, LPVOID reserved)

## API 表面の理解のために参照した情報源 (コードは不使用)

これらは「どのような関数・マクロ・シンボル名が必要とされているか」の
API 表面のみを理解する目的で参照した。コードの複製は行っていない。

- MaxKellermann/mingwrt (GitHub) - Public Domain として配布されているファイル (dllcrt1.c)。
  必要なシンボル名・エントリポイントの挙動を確認するために参照。
- CeGCC メーリングリスト (cegcc-devel) の議論:
  - UNDER_CE, __COREDLL__, _WIN32_WCE マクロの意味
  - arm-wince-mingw32ce ターゲットでのビルド手順
- Stack Overflow / gamedev.net 等での Windows エントリポイント議論
  (WinMainCRTStartup, mainCRTStartup, DllMainCRTStartup の慣習)

## クリーンルーム方針

実装者は既存の CRT ソース (glibc, musl, newlib, MinGW, MinGW-w64, cegcc/mingwrt, MSVC CRT ソース)
を読まず、またそれらをインスパイアして記述しない。すべてのロジックは
上記の一次資料の仕様に基づき新規に設計・記述する。
