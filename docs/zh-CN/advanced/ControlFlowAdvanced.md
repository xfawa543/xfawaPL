# 控制流高阶用法

本文档介绍控制流的高级用法和组合技巧。

---

## 复杂条件组合

### 多条件判断

```xfawa
#complex_conditions {
    fn main() {
        int age = 25
        bool hasLicense = true
        bool hasInsurance = true
        
        if age >= 18 && hasLicense && hasInsurance {
            print("可以合法驾驶")
        }
        else if age >= 18 && hasLicense {
            print("可以驾驶，但需要购买保险")
        }
        else if age >= 18 {
            print("需要考取驾照")
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
        
        // 嵌套条件
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
        
        print(n + "的阶乘是:")
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
        int max = numbers[0]
        
        for num in numbers {
            sum = sum + num
            if num > max {
                max = num
            }
        }
        
        print("总和:")
        print(sum)
        print("最大值:")
        print(max)
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
        bool found = false
        
        for num in arr {
            if num == target {
                found = true
                break
            }
        }
        
        if found {
            print("找到了目标: " + target)
        }
        else {
            print("未找到目标")
        }
    }
}
```

### 提前返回

```xfawa
#early_return {
    fn main() {
        int result = findFirst([5, 3, 8, 1, 9], 8)
        print(result)
    }
    
    fn findFirst(arr, target) {
        for num in arr {
            if num == target {
                return num
            }
        }
        return -1
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
        
        print("斐波那契数列前" + n + "项:")
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
        int num = 17
        if isPrime(num) {
            print(num + "是素数")
        }
        else {
            print(num + "不是素数")
        }
    }
    
    fn isPrime(n) {
        if n < 2 {
            return false
        }
        int i = 2
        while (i * i <= n) {
            if n % i == 0 {
                return false
            }
            i = i + 1
        }
        return true
    }
}
```

### 二分查找

```xfawa
#binary_search {
    fn main() {
        int arr = [1, 3, 5, 7, 9, 11, 13, 15, 17, 19]
        int target = 11
        int result = binarySearch(arr, target)
        
        if result != -1 {
            print("找到目标，索引: " + result)
        }
        else {
            print("未找到目标")
        }
    }
    
    fn binarySearch(arr, target) {
        int left = 0
        int right = 9
        
        while (left <= right) {
            int mid = (left + right) / 2
            if arr[mid] == target {
                return mid
            }
            else if arr[mid] < target {
                left = mid + 1
            }
            else {
                right = mid - 1
            }
        }
        return -1
    }
}
```

### 冒泡排序

```xfawa
#bubble_sort {
    fn main() {
        int arr = [64, 34, 25, 12, 22, 11, 90]
        
        print("排序前:")
        printArray(arr)
        
        bubbleSort(arr)
        
        print("排序后:")
        printArray(arr)
    }
    
    fn bubbleSort(arr) {
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
    }
    
    fn printArray(arr) {
        for item in arr {
            print(item)
        }
    }
}
```

---

## 函数组合模式

### 计算器函数库

```xfawa
#calculator_functions {
    fn main() {
        int a = 10
        int b = 3
        
        print("加法: " + add(a, b))
        print("减法: " + subtract(a, b))
        print("乘法: " + multiply(a, b))
        print("除法: " + divide(a, b))
        print("取模: " + modulo(a, b))
        print("幂运算: " + power(a, b))
    }
    
    fn add(x, y) {
        return x + y
    }
    
    fn subtract(x, y) {
        return x - y
    }
    
    fn multiply(x, y) {
        return x * y
    }
    
    fn divide(x, y) {
        return x / y
    }
    
    fn modulo(x, y) {
        return x % y
    }
    
    fn power(base, exp) {
        int result = 1
        int i = 0
        while (i < exp) {
            result = result * base
            i = i + 1
        }
        return result
    }
}
```

### 递归模式（模拟）

```xfawa
#recursion_simulation {
    fn main() {
        int n = 5
        print(n + "的阶乘: " + factorial(n))
    }
    
    fn factorial(n) {
        if n <= 1 {
            return 1
        }
        int result = 1
        int i = 2
        while (i <= n) {
            result = result * i
            i = i + 1
        }
        return result
    }
}
```

---

## 相关文档

- [控制流基础](../ControlFlow.md)
- [运算符](../Operators.md)
- [内置函数](../Builtins.md)
