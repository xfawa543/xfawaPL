# 数据类型 (Data Types)

本文档介绍 xfawaPL 支持的数据类型。

---

## 基本类型

| 类型 | 关键字 | 说明 | 示例 |
|------|--------|------|------|
| 整数 | `int` | 32位有符号整数 | `int a = 42` |
| 长整数 | `long` | 64位有符号整数 | `long b = 3000000000` |
| 浮点数 | `float` | 单精度浮点数 | `float c = 3.14` |
| 布尔值 | `bool` | 布尔类型 | `bool d = true` |
| 字符串 | `string` | 字符串类型 | `string s = "Hello"` |

---

## 变量声明

### 显式类型声明（推荐）

```xfawa
int a = 1
long b = 3000000000
float c = 3.14
bool d = true
string s = "Hello, xfawaPL!"
```

### 隐式类型声明

```xfawa
a = 1          // 会产生编译警告，建议使用显式类型声明
```

---

## 字面量

### 数字字面量

```xfawa
int num = 42
int max = 2147483647        // 整数最大值
long bigNum = 3000000000    // 超过32位的整数使用 long
long maxLong = 9223372036854775807  // long 最大值
float pi = 3.14159
float scientific = 1.5e10   // 科学计数法
```

### 整数范围

- 整数 (int) 字面量有效范围：`-2147483648` 到 `2147483647`
- 长整数 (long) 字面量有效范围：`-9223372036854775808` 到 `9223372036854775807`
- 负数通过一元减号实现：`int neg = -42`

### 布尔字面量

```xfawa
bool yes = true
bool no = false
```

### 字符串字面量

```xfawa
string text = "Hello, xfawaPL!"
string empty = ""
string multiline = "Line1\nLine2"    // 支持转义字符
```

#### 转义字符

| 转义序列 | 说明 |
|---------|------|
| `\n` | 换行符 |
| `\t` | 制表符 |
| `\r` | 回车符 |
| `\\` | 反斜杠 |
| `\"` | 双引号 |
| `\0` | 空字符 |

---

## 数组

### 数组定义

#### 显式元素列表

```xfawa
int arr1 = [1, 2, 3, 4, 5]
int arr2 = [10, 20, 30]
```

#### 范围语法

```xfawa
int arr1 = [1...10]      // 1到10的数组
int arr2 = [0...100]     // 0到100的数组
```

### 数组索引

```xfawa
int arr = [1, 2, 3, 4, 5]

print(arr[0])      // 访问第一个元素，输出：1
print(arr[2])      // 访问第三个元素，输出：3
print(arr[-1])     // 访问最后一个元素，输出：5
print(arr[-2])     // 访问倒数第二个元素，输出：4
```

### 数组范围

```xfawa
int arr = [1...10]
int sub = arr[2...5]    // 获取索引2到5的子数组
```

### 数组遍历

```xfawa
int arr = [1, 2, 3, 4, 5]

for item in arr {
    print(item)
}
```

---

## 类型转换

xfawaPL 是静态类型语言，类型在编译时确定。

### 隐式转换

```xfawa
int a = 10
float b = a       // 整数隐式转换为浮点数
```

### 显式转换（通过运算）

```xfawa
int a = 10
float b = a * 1.0   // 通过运算转换为浮点数
```

---

## 完整示例

```xfawa
#types_demo {
    fn main() {
        int integer = 42
        float decimal = 3.14159
        bool flag = true
        
        print("Integer:")
        print(integer)
        
        print("Float:")
        print(decimal)
        
        print("Boolean:")
        print(flag)
        
        int arr = [1, 2, 3, 4, 5]
        print("Array first element:")
        print(arr[0])
        
        int range = [1...10]
        print("Range array random element:")
        print(rnd(range))
    }
}
```

---

## 相关文档

- [数据类型高阶用法](./advanced/DataTypesAdvanced.md)
- [基础语法](./BasicSyntax.md)
- [运算符](./Operators.md)
- [内置函数](./Builtins.md)
