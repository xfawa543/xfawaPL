# Control Flow Advanced

This document introduces advanced usage and combined techniques of control flow.

---

## Complex Condition Combinations

### Multi-condition Judgment

```xfawa
#complex_conditions {
    fn main() {
        int age = 25
        int hasLicense = 1
        int hasInsurance = 1
        
        if age >= 18 {
            if hasLicense == 1 {
                if hasInsurance == 1 {
                    print("Can legally drive")
                }
                else {
                    print("Can drive but need insurance")
                }
            }
            else {
                print("Need to get a license")
            }
        }
        else {
            print("Underage, cannot drive")
        }
    }
}
```

### Nested Condition Optimization

```xfawa
#nested_optimization {
    fn main() {
        int score = 85
        int attendance = 90
        
        if score >= 60 {
            if attendance >= 80 {
                print("Pass, good attendance")
            }
            else {
                print("Pass, but insufficient attendance")
            }
        }
        else {
            print("Failed")
        }
    }
}
```

---

## Loop Patterns

### Sum Accumulation

```xfawa
#sum_pattern {
    fn main() {
        int sum = 0
        int i = 1
        
        while (i <= 100) {
            sum = sum + i
            i = i + 1
        }
        
        print("Sum 1 to 100:")
        print(sum)
    }
}
```

### Factorial Calculation

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
        
        print("Factorial of 5:")
        print(result)
    }
}
```

### Array Traversal and Processing

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
        
        print("Sum:")
        print(sum)
        print("Max:")
        print(max_val)
        print("Average:")
        print(sum / 5)
    }
}
```

---

## Loop Control Techniques

### Conditional Break

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
            print("Found target:")
            print(target)
        }
        else {
            print("Target not found")
        }
    }
}
```

### Find First Match

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

## Classic Algorithm Implementations

### Multiplication Table

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

### Fibonacci Sequence

```xfawa
#fibonacci {
    fn main() {
        int n = 10
        int a = 0
        int b = 1
        int count = 0
        
        print("First 10 Fibonacci numbers:")
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

### Prime Check

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
            print("Is prime")
        }
        else {
            print("Not prime")
        }
    }
}
```

---

## Nested Loop Patterns

### 2D Pattern Output

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

### Matrix Traversal Simulation

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

## Combined Examples

### Bubble Sort

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

### Binary Search

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

## Related Documentation

- [Control Flow](../ControlFlow.md)
- [Operators Advanced](./OperatorsAdvanced.md)
- [Data Types Advanced](./DataTypesAdvanced.md)
