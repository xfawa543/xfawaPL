项目结构说明

- `bin/`：已编译的可执行文件（不推荐直接提交到 git，但方便本地使用）
  - `xfawac0.exe` — 旧的 C 后端编译器二进制
  - `xfawac_llvm.exe` — LLVM 后端原型（生成 C++ 再编译）
  - `xfawac_llvm_ir.exe` — LLVM-IR 原型（生成 .ll，再用 llc/g++ 链接）
  - `hello_llvm.exe`, `hello_ir.exe` — 后端生成的示例可执行

- `build/`：构建产物和临时源（可安全清理或加入 `.gitignore`）
  - `temp_*.c`, `temp_*.cpp`, `temp_*.ll`, `*.o`

- `llvm_backend/`：LLVM 后端相关源（原型代码、构建脚本）

- `test/`：示例 `.xf` 源文件（`hello.xf`, `if_test.xf`, `if_test-2.xf`, `duoblock.xf` 等）

- 根目录：
  - `xfawac0.c` — 旧的 C 后端源
  - `run_tests.ps1`, `UPDATE_COMMANDS.txt`, 其他文档

建议：
- 把 `bin/` 添加到 `.gitignore`，只在 release 时打包二进制。
- 定期清理 `build/` 中的临时文件，或把它们移动到 CI 的工作目录中。
