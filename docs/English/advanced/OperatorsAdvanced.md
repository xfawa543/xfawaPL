# Operators Advanced

This document introduces advanced techniques and combined usage of xfawaPL operators.

---

## Compound Expressions

### Chained Comparison

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

### Nested Operations

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

## Arithmetic Operation Techniques

### Power Calculation

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

### GCD Calculation

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

### Digit Extraction

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

## Comparison Operation Techniques

### Range Check

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

### Three-way Compare

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

## Logical Operation Techniques

### Multi-condition Logic

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

### Flag Simulation

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

## Modulo Operation Techniques

### Cyclic Index

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

### Odd/Even Check

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

## Range Operation Techniques

### Range Traversal

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

### Range Sum

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

## Combined Examples

### Expression Evaluator

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

### Comparison Demo

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

## Related Documentation

- [Operators](../Operators.md)
- [Data Types Advanced](./DataTypesAdvanced.md)
- [Control Flow Advanced](./ControlFlowAdvanced.md)
