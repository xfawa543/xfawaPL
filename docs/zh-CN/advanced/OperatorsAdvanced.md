# 运算符高阶用法 (Operators Advanced)

本文档介绍 xfawaPL 运算符的高级技巧和组合用法。

---

## 复合表达式

### 链式比较

```xfawa
#chained_comparison {
    fn main() {
        print("=== Chained Comparison ===")
        
        int x = 15
        
        print("x in [10, 20]:")
        if x >= 10 {
            if x <= 20 {
                print(1)
            }
            else {
                print(0)
            }
        }
        else {
            print(0)
        }
        
        print("x not in [5, 12]:")
        if x < 5 {
            print(1)
        }
        else if x > 12 {
            print(1)
        }
        else {
            print(0)
        }
    }
}
```

### 嵌套运算

```xfawa
#nested_operations {
    fn main() {
        print("=== Nested Operations ===")
        
        int a = 10
        int b = 20
        int c = 30
        
        int result1 = a + b * c - a / b
        int result2 = (a + b) * (c - a) / b
        
        print("a + b * c - a / b =")
        print(result1)
        
        print("(a + b) * (c - a) / b =")
        print(result2)
    }
}
```

---

## 算术运算技巧

### 幂运算模拟

```xfawa
#power {
    fn main() {
        print("=== Power Calculation ===")
        
        int base = 2
        int exp = 10
        int result = 1
        int i = 0
        
        while (i < exp) {
            result = result * base
            i = i + 1
        }
        
        print("2^10 =")
        print(result)
    }
}
```

### 最大公约数

```xfawa
#gcd {
    fn main() {
        print("=== GCD Calculation ===")
        
        int a = 48
        int b = 18
        
        while (b != 0) {
            int temp = b
            b = a % b
            a = temp
        }
        
        print("GCD(48, 18) =")
        print(a)
    }
}
```

### 数字提取

```xfawa
#digit_extraction {
    fn main() {
        print("=== Digit Extraction ===")
        
        int n = 12345
        
        print("Last digit:")
        int last = n % 10
        print(last)
        
        print("Without last digit:")
        int rest = n / 10
        print(rest)
        
        print("Second to last:")
        int second = (n / 10) % 10
        print(second)
    }
}
```

---

## 比较运算技巧

### 范围检查

```xfawa
#range_check {
    fn main() {
        print("=== Range Check ===")
        
        int value = 15
        int min_val = 10
        int max_val = 20
        
        print("Is 15 in [10, 20]?")
        if value >= min_val {
            if value <= max_val {
                print(1)
            }
            else {
                print(0)
            }
        }
        else {
            print(0)
        }
    }
}
```

### 三向比较模拟

```xfawa
#three_way_compare {
    fn main() {
        print("=== Three-way Compare ===")
        
        int a = 15
        int b = 20
        
        print("Comparing 15 and 20:")
        
        if a < b {
            print(-1)
        }
        else if a > b {
            print(1)
        }
        else {
            print(0)
        }
    }
}
```

---

## 逻辑运算技巧

### 多条件判断

```xfawa
#multi_condition {
    fn main() {
        print("=== Multi-condition Logic ===")
        
        int age = 25
        int score = 85
        
        print("Eligible (age >= 18 AND score >= 60)?")
        if age >= 18 {
            if score >= 60 {
                print(1)
            }
            else {
                print(0)
            }
        }
        else {
            print(0)
        }
        
        print("Special (age < 18 OR score >= 90)?")
        if age < 18 {
            print(1)
        }
        else if score >= 90 {
            print(1)
        }
        else {
            print(0)
        }
    }
}
```

### 标志位模拟

```xfawa
#flags {
    fn main() {
        print("=== Flag Simulation ===")
        
        int FLAG_A = 1
        int FLAG_B = 2
        int FLAG_C = 4
        
        int flags = 0
        flags = flags + FLAG_A
        flags = flags + FLAG_C
        
        print("Has FLAG_A:")
        if flags >= FLAG_A {
            int remainder = flags - FLAG_A
            if remainder >= 0 {
                if remainder < FLAG_A {
                    print(1)
                }
                else {
                    int check = flags / FLAG_A
                    if check % 2 == 1 {
                        print(1)
                    }
                    else {
                        print(0)
                    }
                }
            }
            else {
                print(0)
            }
        }
        else {
            print(0)
        }
        
        print("Flags value:")
        print(flags)
    }
}
```

---

## 取模运算技巧

### 循环索引

```xfawa
#cyclic_index {
    fn main() {
        print("=== Cyclic Index ===")
        
        int total = 5
        int current = 3
        
        print("Current:")
        print(current)
        
        print("Next:")
        int next_val = current + 1
        if next_val >= total {
            next_val = 0
        }
        print(next_val)
        
        print("Previous:")
        int prev_val = current - 1
        if prev_val < 0 {
            prev_val = total - 1
        }
        print(prev_val)
    }
}
```

### 奇偶判断

```xfawa
#odd_even {
    fn main() {
        print("=== Odd/Even Check ===")
        
        int arr = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
        
        print("Even numbers:")
        for item in arr {
            if item % 2 == 0 {
                print(item)
            }
        }
        
        print("Odd numbers:")
        for item in arr {
            if item % 2 != 0 {
                print(item)
            }
        }
    }
}
```

---

## 范围运算技巧

### 范围遍历

```xfawa
#range_traversal {
    fn main() {
        print("=== Range Traversal ===")
        
        int range = [1...10]
        
        for i in range {
            print(i)
        }
    }
}
```

### 范围求和

```xfawa
#range_sum {
    fn main() {
        print("=== Range Sum ===")
        
        int total = 0
        int i = 1
        
        while (i <= 10) {
            total = total + i
            i = i + 1
        }
        
        print("Sum 1 to 10:")
        print(total)
    }
}
```

---

## 组合示例

### 数学表达式求值

```xfawa
#expression_evaluator {
    fn main() {
        print("=== Expression Evaluator ===")
        
        int a = 10
        int b = 5
        
        print("10 + 5 =")
        print(a + b)
        
        print("10 - 5 =")
        print(a - b)
        
        print("10 * 5 =")
        print(a * b)
        
        print("10 / 5 =")
        print(a / b)
        
        print("10 % 3 =")
        print(a % 3)
    }
}
```

### 比较演示

```xfawa
#comparison_demo {
    fn main() {
        print("=== Comparison Demo ===")
        
        int a = 10
        int b = 20
        
        print("a = 10, b = 20")
        
        print("a == b:")
        if a == b {
            print(1)
        }
        else {
            print(0)
        }
        
        print("a != b:")
        if a != b {
            print(1)
        }
        else {
            print(0)
        }
        
        print("a < b:")
        if a < b {
            print(1)
        }
        else {
            print(0)
        }
        
        print("a <= b:")
        if a <= b {
            print(1)
        }
        else {
            print(0)
        }
        
        print("a > b:")
        if a > b {
            print(1)
        }
        else {
            print(0)
        }
        
        print("a >= b:")
        if a >= b {
            print(1)
        }
        else {
            print(0)
        }
    }
}
```

---

## 相关文档

- [运算符](../Operators.md)
- [数据类型高阶用法](./DataTypesAdvanced.md)
- [控制流高阶用法](./ControlFlowAdvanced.md)
