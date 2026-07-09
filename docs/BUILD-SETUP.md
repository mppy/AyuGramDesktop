# PurrGram Build Environment — Hướng Dẫn Cài Đặt (VS 2026)

## Tình Trạng Hiện Tại (sau cài lại Windows)

- ✅ D:\TBuild\tdesktop — source code PurrGram (nguyên)
- ✅ D:\TBuild\Libraries — ffmpeg, Qt, breakpad, v.v. (~20GB)
- ✅ D:\TBuild\ThirdParty — msys64, git, python, jom (~3GB)
- ✅ D:\TBuild\tdesktop\out\Release\PurrGram.exe — binary 231MB (chạy được)
- ✅ D:\TBuild\tdesktop\out\Release\tdata — session Telegram (không cần login lại)
- ❌ Visual Studio — bị xóa (cài ổ C)
- ❌ Windows SDK — bị xóa
- ❌ CMake — bị xóa

## Bước 1: Cài VS 2026 Community (18.7.3)

### 1.1 Tải installer
- Trang chính thức: https://visualstudio.microsoft.com/downloads/
- **Chỉ còn VS 2026 18.7.3** — VS 2022 đã hết mainstream support
- Chọn **Visual Studio Community 2026** (free)

### 1.2 Trong installer, chọn:

**Workload (tick):**
- ✅ **Desktop development with C++**

**Individual Components (tick thêm nếu chưa có):**
- ✅ **MSVC v145 - VS 2026 C++ x64/x86 build tools** (default — toolset chính)
- ✅ **MSVC v143 - VS 2022 C++ x64/x86 build tools** (OPTIONAL — fallback nếu v145 có issue)
- ✅ **Windows 10 SDK 10.0.26100.0** (hoặc SDK mới hơn nếu không có 26100)
- ✅ **C++ CMake tools for Windows** (mặc định có)
- ✅ **C++ ATL for v145 build tools** (mặc định có)
- ✅ **C++ AddressSanitizer** (optional, cho debug memory)

**Bỏ tick (tiết kiệm ~5GB):**
- ❌ C++ MFC (PurrGram không dùng)
- ❌ C++/CLI support
- ❌ C++/CX support (deprecated)
- ❌ Windows 11 SDK (Win 10 SDK đủ)
- ❌ .NET desktop development
- ❌ Universal Windows Platform development
- ❌ Mobile development with C++

### 1.3 Install
- Install location: mặc định (C:) — VS bắt buộc cài component hệ thống vào C:, không portable
- Đợi 30-45 min

## Bước 2: Chạy Build Env Restore

```powershell
powershell -ExecutionPolicy Bypass -File D:\TBuild\tdesktop\docs\build-env-restore.ps1
```

Script tự động:
1. Detect VS 2026 + v145 toolset + Win SDK
2. Verify CMake 4.x (tích hợp VS)
3. Add ThirdParty tools (perl, git, python, jom) vào PATH
4. Regenerate CMakeCache (VS 2026 generator, x64, v145)
5. Verify configure thành công
6. Hướng dẫn build tiếp theo

## Bước 3: Build PurrGram

Mở **x64 Native Tools Command Prompt for VS 2026** (Start Menu > Visual Studio 2026):

```cmd
cd /d D:\TBuild\tdesktop\out
cmake --build . --config Release --target Telegram
```

- **Full rebuild lần đầu**: ~60 min (do đổi toolset v143 → v145)
- **Incremental build sau đó**: ~12 min

## Lưu Ý Quan Trọng

1. **Toolset change**: v143 (cũ) → v145 (mới). Binary output tương thích, nhưng lần đầu phải full rebuild.
2. **API credentials**: PurrGram dùng api_id=13994409, api_hash=9d0860cf26be00e72647bd516e648676
3. **CMakeCache backup**: Script tự backup CMakeCache cũ → `CMakeCache.backup.txt`
4. **Nếu build fail**: xem `D:\TBuild\purr-cmake-configure.log`
5. **tdata session**: PurrGram.exe ở D:\TBuild\tdesktop\out\Release\PurrGram.exe chạy được ngay, không cần build lại để test binary cũ
6. **PurrGram.exe đang chạy**: phải tắt trước khi build (file lock)
7. **link.exe hang**: nếu link.exe CPU flat → kill + rebuild
8. **Fallback v143**: nếu v145 build fail, có thể đổi toolset sang v143 (script auto-detect)

## Lần Sau Cài Lại Windows

1. Cài VS 2026 Community (30 min, không tránh được — VS cần component hệ thống)
2. Chạy `build-env-restore.ps1` (2 min, tự động)
3. Build (12-60 min tùy incremental/full)

**Tổng: ~45-90 min.** Không cần remember paths, options, tool versions.

## Files Quan Trọng

| File | Vai trò |
|---|---|
| `D:\TBuild\tdesktop\docs\build-env-restore.ps1` | Script auto-restore build env |
| `D:\TBuild\tdesktop\docs\build-windows-local.ps1` | Script build tự động (có thể dùng thay cho cmd) |
| `D:\TBuild\tdesktop\docs\building-win-x64.md` | Docs build gốc của tdesktop |
| `D:\TBuild\tdesktop\purrgram-roadmap-vi.md` | Roadmap PurrGram |
| `D:\TBuild\tdesktop\out\Release\PurrGram.exe` | Binary chạy được |
| `D:\TBuild\tdesktop\out\Release\tdata\` | Session Telegram (đừng xóa!) |

## Tại Sao Không Dùng VS 2022?

- Microsoft đã ngừng mainstream support VS 2022
- Trang download chính thức chỉ còn VS 2026
- VS 2026 ships v145 toolset (mới hơn, optimized)
- VS 2026 vẫn cài được v143 toolset (fallback) nếu cần
- CMake 4.1.2 tích hợp sẵn (mới hơn, fix bugs)
