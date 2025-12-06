# xfawa/Compiler 更新日志
# xfawa/Compiler Update Log

## 版本 1.0.0-a.3 (Mod试炼 / The Mod Trial)

### 主要更新内容
### Major Update Content

1. **新增功能**
1. **New Features**
   - 实现了对 `//` 格式注释的支持
   - 支持行前注释、行尾注释、连续注释、包含特殊字符的注释等多种注释场景
   - 添加了数组步数功能，支持设置步长，并包含数字界限检查
   - 实现了else if语法支持
   - 添加了代码块调用功能，支持跨块函数调用
   - Implemented support for `//` format comments
   - Supports various comment scenarios: line-leading comments, line-trailing comments, consecutive comments, and comments containing special characters
   - Added array step functionality, supporting step size setting with number boundary checks
   - Implemented else if syntax support
   - Added code block calling functionality, supporting cross-block function calls

2. **移除内容**
2. **Removed Content**
   - 移除了#注释类型的支持（从未实际实现，仅为文档错误提及）
   - Removed support for # comment type (never actually implemented, only a documentation error)

3. **测试文件**
3. **Test Files**
   - 添加了 `comment_test.xf` - 基础注释功能测试
   - 添加了 `comment_test_complete.xf` - 完整注释功能测试
   - Added `comment_test.xf` - Basic comment functionality test
   - Added `comment_test_complete.xf` - Complete comment functionality test

4. **文档更新**
4. **Documentation Updates**
   - 更新了根目录和 `llvm_backend` 目录下的 README.md，添加了版本号和版本称号
   - 更新了 `UPDATE_COMMANDS.txt`，添加了注释功能测试的相关命令
   - Updated README.md in root directory and `llvm_backend` directory, adding version number and version title
   - Updated `UPDATE_COMMANDS.txt`, adding commands related to comment functionality testing

### 编译器改进
### Compiler Improvements

- 在 `parse_and_emit` 函数中添加了对 `//` 格式注释的解析和处理逻辑
- 确保注释内容不会影响代码的正确编译和执行
- 实现了函数定义机制，不限制main命名
- 添加了mod系统支持，但热加载功能被删除
- LLVM后端初步接入LLVM库，实现基本功能
- 完善了LLVM后端对if/else的编译能力
- 注：此版本之后C端（xfawac0.c）将停止更新和维护，但不删除
- Added parsing and processing logic for `//` format comments in the `parse_and_emit` function
- Ensured comment content does not affect correct compilation and execution of code
- Implemented function definition mechanism without restricting main naming
- Added mod system support, but hot reload functionality was removed
- LLVM backend initially integrated with LLVM library, implementing basic functionality
- Improved LLVM backend's ability to compile if/else statements
- Note: After this version, the C side (xfawac0.c) will stop being updated and maintained, but will not be deleted

### 原计划功能完成状态
### Original Planned Features Completion Status

1. **LLVM后端接入真正的LLVM** - ✅ 已部分完成，LLVM后端已接入LLVM库并实现基本功能
2. **if强制定义函数，但不限制main命名** - ✅ 已完成，支持各种函数名作为入口函数
3. **加入数组步数功能，但不能超过设置的数字界限，否则报错** - ✅ 已完成，实现了步数功能和界限检查
4. **加入else if语法支持** - ✅ 已完成，支持else if语法
5. **添加代码块调用功能，语法按照call.xf** - ✅ 已完成，实现了跨块函数调用
6. **mod系统加入，mod有一个简单的热加载功能** - ✅ 已完成，但热加载功能被删除，由于缺少导入的准确官方代码，mod暂时无法使用
7. **完善LLVM后端对if/else的编译能力** - ✅ 已完成，LLVM后端支持if/else语法编译
1. **LLVM backend integration with real LLVM** - ✅ Partially completed, LLVM backend integrated with LLVM library and implemented basic functionality
2. **if强制define函数，但不限制main命名** - ✅ Completed, supports various function names as entry functions
3. **Added array step functionality, but cannot exceed set number boundaries, otherwise error** - ✅ Completed, implemented step functionality and boundary checking
4. **Added else if syntax support** - ✅ Completed, supports else if syntax
5. **Added code block calling functionality, syntax according to call.xf** - ✅ Completed, implemented cross-block function calls
6. **Added mod system with simple hot reload functionality** - ✅ Completed, but hot reload functionality was removed, mod temporarily unavailable due to lack of accurate official import code
7. **Improved LLVM backend's ability to compile if/else** - ✅ Completed, LLVM backend supports if/else syntax compilation

### 使用说明
### Usage Instructions

使用新的注释功能：
Using the new comment functionality:
```
// 这是一行注释
print("Hello") // 这也是注释
```

运行注释测试：
Running comment tests:
```
.\xfawac0.exe test\comment_test.xf
.\comment_test.exe
```

使用代码块调用功能：
Using code block calling functionality:
```
#block { fn func() { print("Function") } }
#main { fn call() { $block@func } }
```

---