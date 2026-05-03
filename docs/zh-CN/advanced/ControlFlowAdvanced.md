# 控制流高阶用法

本文档介绍控制流的高级用法和组合技巧。

---

## 复杂条件组合

### 多条件判断

```xfawa
#complex_conditions {
    fn main() {
        int age = 25
        int hasLicense = 1
        int hasInsurance = 1
        
        if age >= 18 {
            if hasLicense == 1 {
                if hasInsurance == 1 {
                    print("可以合法驾驶")
                }
                else {
                    print("可以驾驶，但需要购买保险")
                }
            }
            else {
                print("需要考取驾照")
            }
        }
        else {
            print("未成年，不能驾驶")
        }
    }
}
```

### 嵌套条件优化

```xfawa
#nested_optimization {
    fn main() {
        int score = 85
        int attendance = 90
        
        if score >= 60 {
            if attendance >= 80 {
                print("成绩合格，出勤良好")
            }
            else {
                print("成绩合格，但出勤不足")
            }
        }
        else {
            print("成绩不合格")
        }
    }
}
```

---

## 循环模式

### 累加求和

```xfawa
#sum_pattern {
    fn main() {
        int sum = 0
        int i = 1
        
        while (i <= 100) {
            sum = sum + i
            i = i + 1
        }
        
        print("1到100的和:")
        print(sum)
    }
}
```

### 阶乘计算

```xfawa
#factorial_pattern {
    fn main() {
        int n = 5
        int result = 1
        int i = 1
        
        while (i <= n) {
            result = result * i
            i = i + 1
        }
        
        print("5的阶乘是:")
        print(result)
    }
}
```

### 数组遍历与处理

```xfawa
#array_processing {
    fn main() {
        int numbers = [10, 20, 30, 40, 50]
        int sum = 0
        int max_val = numbers[0]
        
        for num in numbers {
            sum = sum + num
            if num > max_val {
                max_val = num
            }
        }
        
        print("总和:")
        print(sum)
        print("最大值:")
        print(max_val)
        print("平均值:")
        print(sum / 5)
    }
}
```

---

## 循环控制技巧

### 条件中断

```xfawa
#conditional_break {
    fn main() {
        int arr = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
        int target = 7
        int found = 0
        
        for num in arr {
            if num == target {
                found = 1
                break
            }
        }
        
        if found == 1 {
            print("找到了目标:")
            print(target)
        }
        else {
            print("未找到目标")
        }
    }
}
```

### 查找第一个匹配

```xfawa
#find_first {
    fn main() {
        int arr = [5, 3, 8, 1, 9]
        int target = 8
        int found = 0
        int index = 0
        int i = 0
        
        for num in arr {
            if num == target {
                found = 1
                index = i
                break
            }
            i = i + 1
        }
        
        if found == 1 {
            print("Found at index:")
            print(index)
        }
        else {
            print("Not found")
        }
    }
}
```

---

## 经典算法实现

### 九九乘法表

```xfawa
#multiplication_table {
    fn main() {
        int i = 1
        while (i <= 9) {
            int j = 1
            while (j <= i) {
                print(i * j)
                j = j + 1
            }
            i = i + 1
        }
    }
}
```

### 斐波那契数列

```xfawa
#fibonacci {
    fn main() {
        int n = 10
        int a = 0
        int b = 1
        int count = 0
        
        print("斐波那契数列前10项:")
        while (count < n) {
            print(a)
            int temp = a + b
            a = b
            b = temp
            count = count + 1
        }
    }
}
```

### 素数判断

```xfawa
#prime_check {
    fn main() {
        int n = 17
        int is_prime = 1
        int i = 2
        
        while (i < n) {
            if n % i == 0 {
                is_prime = 0
                break
            }
            i = i + 1
        }
        
        if is_prime == 1 {
            print("是素数")
        }
        else {
            print("不是素数")
        }
    }
}
```

---

## 嵌套循环模式

### 二维模式输出

```xfawa
#pattern_output {
    fn main() {
        int rows = 5
        int i = 1
        
        while (i <= rows) {
            int j = 1
            while (j <= i) {
                print(j)
                j = j + 1
            }
            i = i + 1
        }
    }
}
```

### 矩阵遍历模拟

```xfawa
#matrix_traversal {
    fn main() {
        int rows = 3
        int cols = 4
        int i = 0
        
        while (i < rows) {
            int j = 0
            while (j < cols) {
                print(i * cols + j)
                j = j + 1
            }
            i = i + 1
        }
    }
}
```

---

## 组合示例

### 数字排序（冒泡）

```xfawa
#bubble_sort {
    fn main() {
        int arr = [64, 34, 25, 12, 22, 11, 90]
        int n = 7
        int i = 0
        
        while (i < n - 1) {
            int j = 0
            while (j < n - i - 1) {
                if arr[j] > arr[j + 1] {
                    int temp = arr[j]
                    arr[j] = arr[j + 1]
                    arr[j + 1] = temp
                }
                j = j + 1
            }
            i = i + 1
        }
        
        print("Sorted array:")
        for num in arr {
            print(num)
        }
    }
}
```

### 二分查找

```xfawa
#binary_search {
    fn main() {
        int arr = [1, 3, 5, 7, 9, 11, 13, 15, 17, 19]
        int target = 11
        int left = 0
        int right = 9
        int found = 0
        
        while (left <= right) {
            int mid = (left + right) / 2
            
            if arr[mid] == target {
                found = 1
                print("Found at index:")
                print(mid)
                break
            }
            else if arr[mid] < target {
                left = mid + 1
            }
            else {
                right = mid - 1
            }
        }
        
        if found == 0 {
            print("Not found")
        }
    }
}
```

---

## 相关文档

- [控制流](../ControlFlow.md)
- [运算符高阶用法](./OperatorsAdvanced.md)
- [数据类型高阶用法](./DataTypesAdvanced.md)
