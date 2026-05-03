# Data Types Advanced

This document introduces advanced techniques and combined usage of xfawaPL data types.

---

## Advanced Array Operations

### Array Traversal

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

### Array Index Access

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

### Range Array

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

## Array Operation Techniques

### Array Sum

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

### Array Search

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

### Array Maximum

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

## Type Usage Techniques

### Integer Operations

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

### Boolean Logic

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

## Data Processing Patterns

### Filter Pattern

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

### Count Pattern

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

### Accumulate Pattern

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

## Combined Examples

### Data Analyzer

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

### Random Selector

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

## Related Documentation

- [Data Types](../DataTypes.md)
- [Operators Advanced](./OperatorsAdvanced.md)
- [Control Flow Advanced](./ControlFlowAdvanced.md)
