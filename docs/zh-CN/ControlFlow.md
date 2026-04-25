# 控制流 (Control Flow)

本文档介绍 xfawaPL 的控制流语句。

---

## 条件语句 (if/else)

### 基本 if 语句

```xfawa
if condition {
    // 条件为真时执行
}
```

### if-else 语句

```xfawa
if condition {
    // 条件为真时执行
}
else {
    // 条件为假时执行
}
```

### if-else if-else 链

```xfawa
int score = 75

if score >= 90 {
    print("优秀")
}
else if score >= 80 {
    print("良好")
}
else if score >= 60 {
    print("及格")
}
else {
    print("不及格")
}
```

### 嵌套条件

```xfawa
int a = 10
int b = 20

if a > 0 {
    if b > 0 {
        print("a 和 b 都是正数")
    }
    else {
        print("a 是正数，b 不是")
    }
}
else {
    print("a 不是正数")
}
```

### 复杂条件

```xfawa
int age = 25
bool hasLicense = true

if age >= 18 && hasLicense {
    print("可以驾驶")
}
else {
    print("不能驾驶")
}
```

---

## 循环语句

### while 循环

```xfawa
int i = 0
while (i < 10) {
    print(i)
    i = i + 1
}
```

#### while 循环示例

```xfawa
#while_demo {
    fn main() {
        int count = 0
        int sum = 0
        
        while (count <= 100) {
            sum = sum + count
            count = count + 1
        }
        
        print("1到100的和:")
        print(sum)
    }
}
```

### for-in 循环

#### 遍历数组

```xfawa
int arr = [1, 2, 3, 4, 5]
for item in arr {
    print(item)
}
```

#### 遍历范围

```xfawa
for i in [1...10] {
    print(i)
}
```

#### for-in 循环示例

```xfawa
#for_demo {
    fn main() {
        int numbers = [10, 20, 30, 40, 50]
        
        print("遍历数组:")
        for num in numbers {
            print(num)
        }
        
        print("遍历范围:")
        for i in [1...5] {
            print(i)
        }
    }
}
```

---

## break 语句

`break` 用于跳出循环。

### 示例

```xfawa
#break_demo {
    fn main() {
        int i = 0
        while (i < 100) {
            if i == 5 {
                break    // 当 i 等于 5 时跳出循环
            }
            print(i)
            i = i + 1
        }
        print("循环结束")
    }
}
```

### 在 for-in 中使用 break

```xfawa
#for_break {
    fn main() {
        int arr = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
        
        for num in arr {
            if num == 5 {
                break
            }
            print(num)
        }
    }
}
```

---

## return 语句

`return` 用于从函数返回值。

### 无返回值

```xfawa
fn greet(name) {
    print("Hello, " + name)
    return
}
```

### 有返回值

```xfawa
fn add(a, b) {
    return a + b
}

fn multiply(a, b) {
    return a * b
}
```

### 示例

```xfawa
#return_demo {
    fn main() {
        int result = add(10, 20)
        print(result)    // 输出：30
        
        int product = multiply(5, 6)
        print(product)   // 输出：30
    }
    
    fn add(a, b) {
        return a + b
    }
    
    fn multiply(a, b) {
        return a * b
    }
}
```

---

## 相关文档

- [控制流高阶用法](./advanced/ControlFlowAdvanced.md)
- [运算符](./Operators.md)
- [内置函数](./Builtins.md)
