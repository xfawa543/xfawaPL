# Mods 系统

本文档详细介绍 xfawaPL 的 Mods 模块化扩展系统。

---

## 概述

Mods 系统是 xfawaPL 的模块化扩展机制，允许开发者：

- **修改关键字**：将语言关键字替换为自定义名称
- **添加自定义语法**：创建全新的语法结构
- **导出公共函数**：定义可复用的函数库

---

## Mod 文件

### 文件格式

- **扩展名**: `.xfmod`
- **示例**: `prob.xfmod`, `utils.xfmod`, `math.xfmod`

### 文件位置

Mod 文件通常存放在项目的 `mods/` 目录下，编译器会在预处理阶段自动查找。

### 基本结构

```xfawa
#mod_name {
    fn main() {
        // 语法修改
        "old_keyword" => "new_keyword"
        
        // 公共函数定义
        fn namespace:functionName() {
            // ...
        }
        
        // 内部函数定义
        fn internalFunc() {
            // ...
        }
    }
    
    // 语法添加（在块内，函数外）
    @add {
        name: syntaxName
        syntax: ...
        logic: {
            // ...
        }
    }
}
```

---

## 导入 Mod

### 导入方式

**所有 mod 导入都必须使用 `%import`**，这是唯一的导入方式：

```xfawa
// 方式1：使用文件名（推荐）
%import "prob"           // 自动添加 .xfmod 后缀

// 方式2：使用完整文件名
%import "prob.xfmod"
```

### 重要说明

1. **只能使用 `%import`**：无论是添加新语法的 mod 还是只导出函数的 mod，都必须使用 `%import`
2. **导入语句不能被修改**：`%import` 是预处理指令，不能通过语法修改来改变
3. **预处理阶段执行**：`%import` 在编译的预处理阶段执行，在语法分析之前加载 mod
4. **位置要求**：导入语句必须放在文件顶部，在模块定义之前

### 导入规则

1. Mod 文件必须存在于编译器可访问的路径
2. 文件名区分大小写
3. 多个 mod 按导入顺序加载

### 示例

```xfawa
// 导入单个 mod
%import "prob"

#app {
    fn main() {
        prob 30 50 {
            print("Triggered!")
        }
    }
}
```

```xfawa
// 导入多个 mod
%import "prob"
%import "repeat"
%import "utils"

#app {
    fn main() {
        // 使用多个 mod 的功能
    }
}
```

---

## 语法修改

将 xfawaPL 的关键字替换为自定义名称。

### 语法格式

```xfawa
"原关键字" => "新关键字"
```

### 可修改的关键字

| 关键字 | 说明 |
|--------|------|
| `fn` | 函数定义 |
| `if` | 条件判断 |
| `else` | 否则分支 |
| `while` | 循环 |
| `break` | 跳出循环 |
| `return` | 返回 |
| `true` | 布尔真 |
| `false` | 布尔假 |
| `print` | 打印输出 |

### 示例

创建 `keywords.xfmod`：

```xfawa
#keywords {
    fn main() {
        "print" => "say"
        "if" => "when"
        "else" => "otherwise"
    }
}
```

使用修改后的语法：

```xfawa
%import "keywords"

#app {
    fn main() {
        when true {
            say("True!")
        }
        otherwise {
            say("False!")
        }
    }
}
```

### 冲突处理

当多个 mod 尝试修改同一关键字时，会产生冲突，编译器会报错提示。

---

## 语法添加 (@add)

使用 `@add` 指令添加全新的语法结构。

### 语法格式

```xfawa
@add {
    name: <语法名称>
    syntax: <语法模式>
    logic: {
        <逻辑实现>
    }
}
```

### 参数占位符

| 占位符 | 说明 |
|--------|------|
| `<param_name>` | 普通参数 |
| `<action>` | 代码块参数 |

### 示例：概率执行语法

创建 `prob.xfmod`：

```xfawa
#prob_syntax {
    @add {
        name: prob
        syntax: prob <min_value> <max_value> { <action> }
        logic: {
            int prob_p = rnd(<min_value>, <max_value>)
            int prob_r = rnd(1, 100)
            if prob_r <= prob_p {
                <action>
            }
        }
    }
}
```

使用：

```xfawa
%import "prob"

#test {
    fn main() {
        prob 30 50 {
            print("30-50% chance triggered!")
        }
    }
}
```

### 限制

- 单个 mod 最多添加 256 个自定义语法
- 语法名称必须唯一

---

## 公共函数导出

定义可被外部调用的公共函数。

### 语法格式

```xfawa
fn <命名空间>:<函数名>() {
    // 函数体
}
```

### 示例

创建 `utils.xfmod`：

```xfawa
#utils {
    fn main() {
        fn pub:greet(name) {
            print("Hello, " + name + "!")
        }
        
        fn pub:add(a, b) {
            return a + b
        }
    }
}
```

使用：

```xfawa
%import "utils"

#app {
    fn main() {
        pub:greet("World")
        
        int result = pub:add(10, 20)
        print(result)
    }
}
```

---

## 内部函数

不带命名空间的函数仅在 mod 内部可见。

### 示例

```xfawa
#pub_with_internal {
    fn main() {
        fn pub:publicFunc() {
            print("Public function")
            helper()
        }
        
        fn helper() {
            print("Internal helper")
        }
    }
}
```

---

## 命名空间

### 保留命名空间

以下命名空间被系统保留，用户无法使用：

| 命名空间 | 用途 |
|----------|------|
| `core` | 核心功能 |
| `sys` | 系统功能 |
| `runtime` | 运行时功能 |
| `compiler` | 编译器功能 |
| `internal` | 内部功能 |
| `builtin` | 内置功能 |
| `std` | 标准库 |
| `ext` | 扩展功能 |
| `lib` | 库功能 |

### 命名空间规则

- 不能使用保留命名空间
- 不能使用多级命名空间（如 `pub:xxx:xxx`）

### 有效示例

```xfawa
fn pub:myFunction() {}      // 有效
fn mylib:helper() {}        // 有效
fn utils:process() {}       // 有效
```

### 无效示例

```xfawa
fn core:test() {}           // 错误：core 是保留命名空间
fn pub:sub:func() {}        // 错误：多级命名空间
```

---

## 相关文档

- [Mods 高阶用法](./advanced/ModsAdvanced.md)
- [基础语法](./BasicSyntax.md)
- [内置函数](./Builtins.md)
- [窗口系统](./WindowSystem.md)
