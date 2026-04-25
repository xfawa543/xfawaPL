# Control Flow Advanced Usage

This document introduces advanced usage and combination techniques for control flow.

---

## Complex Condition Combinations

### Multiple Condition Judgment

```xfawa
#complex_conditions {
    fn main() {
        int age = 25
        bool hasLicense = true
        bool hasInsurance = true
        
        if age >= 18 && hasLicense && hasInsurance {
            print("Can legally drive")
        }
        else if age >= 18 && hasLicense {
            print("Can drive, but need insurance")
        }
        else if age >= 18 {
            print("Need to get a license")
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
        
        // Nested conditions
        if score >= 60 {
            if attendance >= 80 {
                print("Passing grade, good attendance")
            }
            else {
                print("Passing grade, but poor attendance")
            }
        }
        else {
            print("Failing grade")
        }
    }
}
```

---

## Loop Patterns

### Accumulation and Summation

```xfawa
#sum_pattern {
    fn main() {
        int sum = 0
        int i = 1
        
        while (i <= 100) {
            sum = sum + i
            i = i + 1
        }
        
        print("Sum of 1 to 100:")
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
        
        print("Factorial of " + n + ":")
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
        int max = numbers[0]
        
        for num in numbers {
            sum = sum + num
            if num > max {
                max = num
            }
        }
        
        print("Sum:")
        print(sum)
        print("Maximum:")
        print(max)
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
        bool found = false
        
        for num in arr {
            if num == target {
                found = true
                break
            }
        }
        
        if found {
            print("Found target: " + target)
        }
        else {
            print("Target not found")
        }
    }
}
```

### Early Return

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
        
        print("First " + n + " Fibonacci numbers:")
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

### Prime Number Check

```xfawa
#prime_check {
    fn main() {
        int num = 17
        if isPrime(num) {
            print(num + " is a prime number")
        }
        else {
            print(num + " is not a prime number")
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

### Binary Search

```xfawa
#binary_search {
    fn main() {
        int arr = [1, 3, 5, 7, 9, 11, 13, 15, 17, 19]
        int target = 11
        int result = binarySearch(arr, target)
        
        if result != -1 {
            print("Found target at index: " + result)
        }
        else {
            print("Target not found")
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

### Bubble Sort

```xfawa
#bubble_sort {
    fn main() {
        int arr = [64, 34, 25, 12, 22, 11, 90]
        
        print("Before sorting:")
        printArray(arr)
        
        bubbleSort(arr)
        
        print("After sorting:")
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

## Function Combination Patterns

### Calculator Function Library

```xfawa
#calculator_functions {
    fn main() {
        int a = 10
        int b = 3
        
        print("Add: " + add(a, b))
        print("Subtract: " + subtract(a, b))
        print("Multiply: " + multiply(a, b))
        print("Divide: " + divide(a, b))
        print("Modulo: " + modulo(a, b))
        print("Power: " + power(a, b))
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

### Recursion Pattern (Simulated)

```xfawa
#recursion_simulation {
    fn main() {
        int n = 5
        print("Factorial of " + n + ": " + factorial(n))
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

## Related Documents

- [Control Flow Basics](../ControlFlow.md)
- [Operators](../Operators.md)
- [Built-in Functions](../Builtins.md)
