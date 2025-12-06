
XF AWA Compiler (xfawac) — 简要说明 / Brief

版本号：1.0.0-a.3
版本称号：Mod试炼

中文说明

- 项目概览：本仓库包含一个小型的 XF → C 的翻译器实现（旧的 C 后端）以及正在试验中的 LLVM 后端原型（C++ 实现）。
- 主要文件：
  - `xfawac0.c`：旧的 C 后端源代码（位于根目录），编译器二进制 `xfawac0.exe` 也放在根目录以便使用。
  - `llvm_backend/`：LLVM 后端原型的源码与构建脚本（`xfawac_llvm.cpp`, `xfawac_llvm_ir.cpp` 等）。默认这些 LLVM 后端工具保留在 `llvm_backend/`（或 `bin/`），以便与旧的 C 后端区分。
  - `test/`：示例 `.xf` 文件（`hello.xf`, `if_test.xf`, `if_test-2.xf`, `duoblock.xf`）。
- 快速使用（Windows PowerShell）：
  - 使用旧的 C 后端：
    ``powershell
    .\xfawac0.exe test\hello.xf -o hello.exe
    .\hello.exe
    ``
  - 使用 LLVM 后端原型（在 `llvm_backend/`）：
    ``powershell
    cd llvm_backend
    .\build.ps1    # 或者手动用 clang++/g++ 编译 xfawac_llvm.cpp
    .\xfawac_llvm.exe ..\test\hello.xf -o ..\hello_llvm.exe
    ..\hello_llvm.exe
    ``
- 许可证：本项目使用 MIT 许可证（详见 `LICENSE`），版权所有人：`xfawa543`。

English (brief)

Version: 1.0.0-a.3
Version Title: The Mod Trial

- Project overview: This repository contains a small XF→C translator (legacy C backend) and experimental LLVM backend prototypes (implemented in C++).
- Key files:
  - `xfawac0.c`: legacy C backend source in the project root; `xfawac0.exe` is kept at the root for convenience.
  - `llvm_backend/`: contains LLVM backend prototypes (`xfawac_llvm.cpp`, `xfawac_llvm_ir.cpp`) and build scripts. These tools are intentionally kept in `llvm_backend/` (or `bin/`) to separate them from the legacy C backend.
  - `test/`: example `.xf` source files (`hello.xf`, `if_test.xf`, `if_test-2.xf`, `duoblock.xf`).
- Quick start (Windows PowerShell):
  - Legacy C backend:
    ```powershell
    .\xfawac0.exe test\hello.xf -o hello.exe
    .\hello.exe
    ```
  - LLVM backend prototype (in `llvm_backend`):
    ```powershell
    cd llvm_backend
    .\build.ps1
    .\xfawac_llvm.exe ..\test\hello.xf -o ..\hello_llvm.exe
    ..\hello_llvm.exe
    ```
- License: MIT (see `LICENSE`), Copyright (c) 2025 `xfawa543`.
