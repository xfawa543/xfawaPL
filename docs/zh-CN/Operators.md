# 运算符 (Operators)

本文档介绍 xfawaPL 支持的各种运算符。

---

## 算术运算符

| 运算符 | 说明 | 示例 | 结果 |
|--------|------|------|------|
| `+` | 加法 | `5 + 3` | `8` |
| `-` | 减法 | `5 - 3` | `2` |
| `*` | 乘法 | `5 * 3` | `15` |
| `/` | 除法 | `6 / 2` | `3` |
| `%` | 取模 | `7 % 3` | `1` |

### 示例

```xfawa
#arithmetic {
    fn main() {
        int a = 10
        int b = 3
        
        print(a + b)    // 输出：13
        print(a - b)    // 输出：7
        print(a * b)    // 输出：30
        print(a / b)    // 输出：3
        print(a % b)    // 输出：1
    }
}
```

---

## 比较运算符

| 运算符 | 说明 | 示例 | 结果 |
|--------|------|------|------|
| `==` | 等于 | `5 == 5` | `true` |
| `!=` | 不等于 | `5 != 3` | `true` |
| `<` | 小于 | `3 < 5` | `true` |
| `<=` | 小于等于 | `3 <= 3` | `true` |
| `>` | 大于 | `5 > 3` | `true` |
| `>=` | 大于等于 | `5 >= 5` | `true` |

### 示例

```xfawa
#comparison {
    fn main() {
        int a = 10
        int b = 20
        
        print(a == b)   // 输出：false
        print(a != b)   // 输出：true
        print(a < b)    // 输出：true
        print(a <= b)   // 输出：true
        print(a > b)    // 输出：false
        print(a >= b)   // 输出：false
    }
}
```

---

## 逻辑运算符

| 运算符 | 说明 | 示例 | 结果 |
|--------|------|------|------|
| `&&` | 逻辑与 | `true && false` | `false` |
| `\|\|` | 逻辑或 | `true \|\| false` | `true` |
| `!` | 逻辑非 | `!true` | `false` |

### 示例

```xfawa
#logical {
    fn main() {
        bool a = true
        bool b = false
        
        print(a && b)   // 输出：false
        print(a || b)   // 输出：true
        print(!a)       // 输出：false
        print(!b)       // 输出：true
    }
}
```

### 短路求值

```xfawa
#short_circuit {
    fn main() {
        int x = 0
        
        // && 短路：如果左边为 false，右边不执行
        if false && (x = 1) == 1 {
            print("Never reached")
        }
        print(x)    // 输出：0，x 未被修改
        
        // || 短路：如果左边为 true，右边不执行
        if true || (x = 2) == 2 {
            print("Reached")
        }
        print(x)    // 输出：0，x 未被修改
    }
}
```

---

## 赋值运算符

| 运算符 | 说明 | 示例 |
|--------|------|------|
| `=` | 赋值 | `a = 10` |

### 示例

```xfawa
#assignment {
    fn main() {
        int a = 10
        int b = a
        print(b)    // 输出：10
    }
}
```

---

## 范围运算符

| 运算符 | 说明 | 示例 |
|--------|------|------|
| `...` | 范围 | `[1...10]` |

### 示例

```xfawa
#range {
    fn main() {
        int arr = [1...10]      // 创建 1 到 10 的数组
        for i in arr {
            print(i)
        }
    }
}
```

---

## 运算符优先级

从高到低：

1. `!` (逻辑非)
2. `*`, `/`, `%` (乘法、除法、取模)
3. `+`, `-` (加法、减法)
4. `<`, `<=`, `>`, `>=` (比较)
5. `==`, `!=` (相等比较)
6. `&&` (逻辑与)
7. `||` (逻辑或)
8. `=` (赋值)

### 示例

```xfawa
#precedence {
    fn main() {
        int a = 2 + 3 * 4       // 先乘后加，结果：14
        int b = (2 + 3) * 4     // 括号优先，结果：20
        
        bool c = true && false || true   // && 优先于 ||，结果：true
        bool d = true && (false || true) // 括号优先，结果：true
        
        print(a)
        print(b)
        print(c)
        print(d)
    }
}
```

---

## 完整示例

```xfawa
#operators_demo {
    fn main() {
        int a = 15
        int b = 4
        
        print("算术运算:")
        print(a + b)
        print(a - b)
        print(a * b)
        print(a / b)
        print(a % b)
        
        print("比较运算:")
        print(a > b)
        print(a < b)
        print(a == b)
        
        print("逻辑运算:")
        print(true && false)
        print(true || false)
        print(!true)
        
        print("范围运算:")
        int range = [1...5]
        for i in range {
            print(i)
        }
    }
}
```

---

## 相关文档

- [运算符高阶用法](./advanced/OperatorsAdvanced.md)
- [数据类型](./DataTypes.md)
- [控制流](./ControlFlow.md)
