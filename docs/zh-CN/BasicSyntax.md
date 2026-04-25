# 基础语法 (Basic Syntax)

本文档介绍 xfawaPL 的基础语法结构。

---

## 代码块 (Block)

代码块是 xfawaPL 的基本组织单元，使用 `#` 符号定义。

### 语法

```xfawa
#block_name {
    // 代码内容
}
```

### 规则

- 块名称必须是有效标识符
- 一个源文件可以包含多个代码块
- 每个代码块可以包含函数定义

### 示例

```xfawa
#MainModule {
    fn main() {
        print("Hello from MainModule")
    }
}

#HelperModule {
    fn helper() {
        print("Helper function")
    }
}
```

---

## 函数定义 (Function)

使用 `fn` 关键字定义函数。

### 语法

```xfawa
fn function_name(param1, param2) {
    // 函数体
}
```

### 规则

- 函数必须在代码块内定义
- 一个代码块内不能定义两个同级函数（可在函数内嵌套定义）
- `main` 函数是程序入口点
- 参数列表可以为空

### 示例

```xfawa
#app {
    fn main() {
        print("Entry point")
    }
    
    fn greet(name) {
        print(name)
    }
}
```

---

## 注释 (Comments)

支持单行注释和多行注释。

### 单行注释

```xfawa
// 这是单行注释
print("Hello") // 行尾注释
```

### 多行注释

```xfawa
/* 
 * 这是多行注释
 * 可以跨越多行
 * 可以包含任何字符：@#$%^&*()
 */
```

### 示例

```xfawa
#example {
    fn main() {
        // 这是单行注释
        print("Hello") // 行尾注释
        
        /* 
         * 这是多行注释
         * 可以包含任何字符
         */
    }
}
```

---

## 程序入口

每个 xfawaPL 程序都需要一个 `main` 函数作为入口点。

### 最简程序

```xfawa
#hello {
    fn main() {
        print("Hello, World!")
    }
}
```

### 多代码块程序

```xfawa
#app {
    fn main() {
        print("程序开始")
        helper:sayHello()
        print("程序结束")
    }
}

#helper {
    fn main() {
        fn pub:sayHello() {
            print("Hello from helper!")
        }
    }
}
```

---

## 无分号设计

xfawaPL 不需要分号结尾，每行语句独立成行。

```xfawa
#example {
    fn main() {
        int a = 1
        int b = 2
        int c = a + b
        print(c)
    }
}
```

---

## 相关文档

- [基础语法高阶用法](./advanced/BasicSyntaxAdvanced.md)
- [数据类型](./DataTypes.md)
- [运算符](./Operators.md)
- [控制流](./ControlFlow.md)
