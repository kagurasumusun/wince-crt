# wince-crt 設計書

## 目的

本ライブラリは、Windows CE (Windows Embedded Compact) を対象とした C Runtime Library (CRT) の
クリーンルーム実装であり、cegcc の mingwrt の代替として、LLVM/Clang (clang, lld, llvm-ar 等)
ツールチェーンで使用することを目的とする。

## クリーンルーム原則の遵守

- ソースコードは、公開規格・Microsoft MSDN 公式文書・ARM AAPCS 仕様・PE/COFF 仕様などの
  一次資料に基づき、独自に設計・記述する。
- 既存のオープンソース実装 (mingwrt, newlib, glibc, musl, cegcc, mingw-w64 等) のソースコードの
  コピー・流用・改変は行わない。
- Public Domain で公開されているファイル (MaxKellermann/mingwrt の一部など) については、
  API の表面(どのような関数シグネチャ・マクロが必要か)を理解するための情報源として
  参照するにとどめ、コードをそのまま複製しない。
- 実装の出典・参照した一次資料については SOURCES.md に記録する。

## ライセンス

本コードは MIT License または同等の寛容なライセンス (ファイル毎に明示) で配布し、
ユーザーが自前のライセンスを持てるようにする。

## 対象アーキテクチャ

初回の実装対象は ARM (ARMv4I, ARMv5, ARMv6, ARMv7, Thumb/Thumb2) とする。
ただし、x86, MIPS, SH についても将来的な拡張を妨げない設計とする。

## 対象 Windows CE バージョン

- Windows CE 3.0 (Pocket PC 2002 相当)
- Windows CE 4.x (.NET)
- Windows CE 5.0
- Windows Embedded CE 6.0
- Windows Embedded Compact 7

既定のターゲットは Windows CE 5.0 以降。

## プリセットマクロ

Clang ターゲット時に本 CRT が規定する事前定義マクロ:

| マクロ          | 値       | 説明                                              |
| --------------- | -------- | ------------------------------------------------- |
| `UNDER_CE`      | 1        | Windows CE ターゲットであることを示す             |
| `_WIN32_WCE`    | 0x501 相当 | Win32 API バージョン (ユーザーが上書き可能)       |
| `__COREDLL__`   | 1        | CRT が coredll.dll に存在することを示す           |
| `_WIN32`        | 1        | Win32 互換                                        |
| `_M_ARM` / `_M_IX86` 等 | アーキテクチャ依存 | MSVC 互換のアーキテクチャマクロ             |
| `__STDC__`      | 1        | 標準準拠                                          |

## エントリポイント

Windows CE 実行可能ファイルの PE エントリポイントは `WinMainCRTStartup` (GUI/Unicode は
`wWinMainCRTStartup`) である。DLL のエントリポイントは `DllMainCRTStartup` である。
コンソールモードは存在するが Windows CE ではデフォルトが GUI であるため、WinMain を基本とする。
ただし main() エントリもサポートする (cegcc との互換性のため)。

### WinMainCRTStartup の動作

1. 自身の HINSTANCE を `GetModuleHandleW(NULL)` で取得
2. コマンドライン文字列を `GetCommandLineW()` で取得
3. atexit テーブル初期化
4. グローバルコンストラクタの呼び出し (.init_array / .ctors 経由)
5. `WinMain(hInstance, NULL, cmdLine, nCmdShow)` を呼び出す
   - nCmdShow は Windows CE では SW_SHOW (5) 相当
6. WinMain からの戻り値で `ExitProcess()` を呼び出す

### DllMainCRTStartup の動作

DLL_PROCESS_ATTACH/DETACH, DLL_THREAD_ATTACH/DETACH をハンドルし、必要ならば
グローバルコンストラクタ/デストラクタを呼び出した後にユーザー定義 `DllMain` を呼ぶ。

## ABI (ARM)

Windows CE on ARM は古い APCS (ARM Procedure Call Standard, 旧 ABI = "Old ABI") を使用する。
Windows Embedded Compact 7 以降は EABI もサポートするが、幅広い互換性のため
APCS (APCS-32, soft-float, 構造体が小さいとレジスタ渡し) を基本とし、
コンパイラフラグによる EABI オプションも許容する。

Clang 側では `-target armv7-windows-gnu -fshort-wchar` 等を指定して ABI を合わせる。

### レジスタ使用法 (APCS)

- r0-r3: 引数/スクラッチ (caller-saved)
- r4-r11: callee-saved
- r12 (IP): スクラッチ
- r13 (SP): スタックポインタ (full descending)
- r14 (LR): リンクレジスタ
- r15 (PC): プログラムカウンタ
- スタックは 4 バイト (APCS) または 8 バイト (EABI) アライメント

### 基本データ型サイズ

ARM (Windows CE):

| 型          | サイズ (Bytes) | 備考              |
| ----------- | -------------- | ----------------- |
| char        | 1              | signed char       |
| short       | 2              |                   |
| int         | 4              |                   |
| long        | 4              |                   |
| long long   | 8              |                   |
| float       | 4              |                   |
| double      | 8              |                   |
| long double | 8              | MSVC 互換 = double |
| ポインタ    | 4              |                   |
| size_t      | 4              | unsigned          |
| ptrdiff_t   | 4              | signed            |
| wchar_t     | 2              | UTF-16 (-fshort-wchar) |

Windows CE は UTF-16LE をネイティブとするため、wchar_t は 2 バイト (unsigned short) とする。

## PE/COFF

- マシンタイプ: IMAGE_FILE_MACHINE_ARM (0x01c0) / IMAGE_FILE_MACHINE_ARMNT (0x01c4)
- Subsystem: IMAGE_SUBSYSTEM_WINDOWS_CE_GUI (9)
- Windows CE ではイメージベースは通常 0x00010000 または 0x10000000
- DLL 名 (coredll.dll) はインポートテーブルで固定
- リロケーションセクション必須 (.reloc)

## インポート (coredll.dll 等)

CRT は基本的に coredll.dll がエクスポートする API を直接呼び出す。
- メモリ: `LocalAlloc/LocalFree/LocalReAlloc` (coredll 内のヒープ経由)、または `HeapAlloc/HeapFree/GetProcessHeap`
- ファイルI/O: `CreateFileW/ReadFile/WriteFile/CloseHandle`
- プロセス: `ExitProcess`, `GetModuleHandleW`, `GetCommandLineW`
- etc.

## メモリ割り当て (malloc/free)

coredll.dll の `LocalAlloc(LPTR, size)` および `LocalFree(handle)` を利用する。
メタデータ(ブロックヘッダ)をアラインメント確保したアドレスの前に置くことで実装する。

## 提供する機能

- C89/C99 標準ライブラリ関数の主要サブセット
  - stdio.h: printf, sprintf, fprintf, vprintf, vsprintf, putchar 等の最小セット
    (Windows CE ではコンソールが存在しないため stdin/stdout/stderr は NUL デバイスを初期化し、
     OutputDebugString やシリアルポートへフォールバック可能とする)
  - stdlib.h: malloc/free/calloc/realloc, atoi/atol/strtol/strtoul, abort/exit/atexit, qsort, bsearch, abs/labs, rand/srand
  - string.h: memcpy/memmove/memset/memcmp/memchr, strlen/strcpy/strncpy/strcat/strncat/strcmp/strncmp/strchr/strrchr/strstr/strspn/strcspn/strpbrk/strtok 等
  - ctype.h: isalpha/isdigit/isspace/islower/isupper/toupper/tolower 等
  - math.h: 基本的な数学関数 (スタブを含む; 一部はコンパイラ組込みで代替可能)
  - time.h: time/clock および最小限の変換関数
  - setjmp.h: setjmp/longjmp (ARM APCS に準拠)
  - stdarg.h: 可変長引数マクロ (clang builtin を使用)
  - stddef.h, limits.h, float.h, stdint.h, stdbool.h, errno.h
- Win32 API ヘッダの最小セット (windows.h および必要なサブセット)
- コンパイラが暗黙に呼出すヘルパ関数 (__chkstk, _alloca, __rt_sdiv 等)
- スタートアップコード (EXE 用, DLL 用)
- atexit 機構
- .ctors / .init_array からのグローバルコンストラクタ呼出し

## 実装方針

- 可能な限り標準 C で記述し、必要なアーキテクチャ依存コード (setjmp/longjmp, __chkstk, スレッドローカル) はアセンブリで分離する。
- 浮動小数点エミュレーションは行わない (OS/ハードウェア任せ、または libgcc/compiler-rt を利用)。
- スレッドローカルストレージは初期バージョンでは最小限のサポートとする。
- C++ サポート (new/delete, グローバルコンストラクタ) はスタートアップコードで .init_array/.ctors を呼び出すことで実現する。
