# 数据类型高阶用法 (Data Types Advanced)

本文档介绍 xfawaPL 数据类型的高级技巧和组合用法。

---

## 数组高级操作

### 数组遍历

```xfawa
#array_traversal {
    fn main() {
        print("=== Array Traversal ===")
        
        int arr = [1, 2, 3, 4, 5]
        
        for item in arr {
            print(item)
        }
    }
}
```

### 数组索引访问

```xfawa
#array_index {
    fn main() {
        print("=== Array Index ===")
        
        int arr = [10, 20, 30, 40, 50]
        
        print("First element:")
        print(arr[0])
        
        print("Third element:")
        print(arr[2])
        
        print("Last element:")
        print(arr[-1])
        
        print("Second to last:")
        print(arr[-2])
    }
}
```

### 范围数组

```xfawa
#range_array {
    fn main() {
        print("=== Range Array ===")
        
        int range = [1...10]
        
        for i in range {
            print(i)
        }
    }
}
```

---

## 数组操作技巧

### 数组求和

```xfawa
#array_sum {
    fn main() {
        print("=== Array Sum ===")
        
        int arr = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
        int total = 0
        
        for item in arr {
            total = total + item
        }
        
        print("Sum:")
        print(total)
    }
}
```

### 数组查找

```xfawa
#array_search {
    fn main() {
        print("=== Array Search ===")
        
        int arr = [5, 12, 8, 23, 17, 9, 31, 6]
        int target = 17
        int found = 0
        int i = 0
        
        for item in arr {
            if item == target {
                found = 1
                print("Found at index:")
                print(i)
            }
            i = i + 1
        }
        
        if found == 0 {
            print("Not found")
        }
    }
}
```

### 数组最大值

```xfawa
#array_max {
    fn main() {
        print("=== Array Max ===")
        
        int arr = [15, 42, 8, 23, 67, 31, 9]
        int max_val = arr[0]
        
        for item in arr {
            if item > max_val {
                max_val = item
            }
        }
        
        print("Maximum value:")
        print(max_val)
    }
}
```

---

## 类型使用技巧

### 整数运算

```xfawa
#int_operations {
    fn main() {
        print("=== Integer Operations ===")
        
        int a = 17
        int b = 5
        
        print("Addition:")
        print(a + b)
        
        print("Subtraction:")
        print(a - b)
        
        print("Multiplication:")
        print(a * b)
        
        print("Division:")
        print(a / b)
        
        print("Modulo:")
        print(a % b)
    }
}
```

### 布尔逻辑

```xfawa
#bool_logic {
    fn main() {
        print("=== Boolean Logic ===")
        
        bool a = true
        bool b = false
        
        print("a AND b:")
        if a && b {
            print(1)
        }
        else {
            print(0)
        }
        
        print("a OR b:")
        if a || b {
            print(1)
        }
        else {
            print(0)
        }
        
        print("NOT a:")
        if !a {
            print(1)
        }
        else {
            print(0)
        }
        
        print("NOT b:")
        if !b {
            print(1)
        }
        else {
            print(0)
        }
    }
}
```

---

## 数据处理模式

### 过滤器模式

```xfawa
#filter_pattern {
    fn main() {
        print("=== Filter Pattern ===")
        print("Even numbers in [1,2,3,4,5,6,7,8,9,10]:")
        
        int arr = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
        
        for item in arr {
            if item % 2 == 0 {
                print(item)
            }
        }
    }
}
```

### 计数模式

```xfawa
#count_pattern {
    fn main() {
        print("=== Count Pattern ===")
        
        int arr = [3, 7, 12, 5, 18, 9, 24, 6, 15]
        int count = 0
        
        for item in arr {
            if item > 10 {
                count = count + 1
            }
        }
        
        print("Count of items > 10:")
        print(count)
    }
}
```

### 累积模式

```xfawa
#accumulate_pattern {
    fn main() {
        print("=== Accumulate Pattern ===")
        
        int arr = [2, 4, 6, 8, 10]
        int product = 1
        
        for item in arr {
            product = product * item
        }
        
        print("Product of all elements:")
        print(product)
    }
}
```

---

## 组合示例

### 数据分析器

```xfawa
#data_analyzer {
    fn main() {
        print("=== Data Analysis ===")
        
        int data = [12, 45, 67, 23, 89, 34, 56, 78, 90, 11]
        
        int sum = 0
        int max_val = data[0]
        int min_val = data[0]
        int count = 0
        
        for item in data {
            sum = sum + item
            count = count + 1
            
            if item > max_val {
                max_val = item
            }
            
            if item < min_val {
                min_val = item
            }
        }
        
        print("Count:")
        print(count)
        
        print("Sum:")
        print(sum)
        
        print("Max:")
        print(max_val)
        
        print("Min:")
        print(min_val)
        
        print("Range:")
        print(max_val - min_val)
    }
}
```

### 随机选择器

```xfawa
#random_selector {
    fn main() {
        print("=== Random Selector ===")
        
        int options = [1, 2, 3, 4, 5]
        
        print("Random selection:")
        print(rnd(options))
        
        print("Random from range [1, 100]:")
        print(rnd(1, 100))
    }
}
```

---

## 相关文档

- [数据类型](../DataTypes.md)
- [运算符高阶用法](./OperatorsAdvanced.md)
- [控制流高阶用法](./ControlFlowAdvanced.md)
