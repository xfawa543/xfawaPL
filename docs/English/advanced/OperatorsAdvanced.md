# Operators Advanced

This document covers advanced techniques and combined usage of xfawaPL operators.

---

## Compound Expressions

### Chained Comparisons

```xfawa
#chained_comparison {
    fn main() {
        int x = 15
        
        bool in_range = x >= 10 && x <= 20
        bool out_range = x < 5 || x > 25
        
        print("x in [10, 20]: " + in_range)
        print("x not in [5, 25]: " + out_range)
        
        fn is_valid_age(age) {
            return age >= 0 && age <= 150
        }
        
        fn is_valid_score(score) {
            return score >= 0 && score <= 100
        }
        
        print("Age 25 valid: " + is_valid_age(25))
        print("Score 85 valid: " + is_valid_score(85))
    }
}
```

### Nested Operations

```xfawa
#nested_operations {
    fn main() {
        int a = 10
        int b = 20
        int c = 30
        
        int result1 = a + b * c - a / b
        int result2 = (a + b) * (c - a) / b
        
        print("a + b * c - a / b = " + result1)
        print("(a + b) * (c - a) / b = " + result2)
        
        bool complex = (a < b) && (b < c) && (a + b > c / 2)
        print("Complex condition: " + complex)
    }
}
```

---

## Advanced Arithmetic Techniques

### Fast Power Computation

```xfawa
#power {
    fn main() {
        fn power(base, exp) {
            int result = 1
            int i = 0
            while i < exp {
                result = result * base
                i = i + 1
            }
            return result
        }
        
        fn fast_power(base, exp) {
            int result = 1
            while exp > 0 {
                if exp % 2 == 1 {
                    result = result * base
                }
                base = base * base
                exp = exp / 2
            }
            return result
        }
        
        print("2^10 = " + power(2, 10))
        print("2^10 (fast) = " + fast_power(2, 10))
        print("3^5 = " + power(3, 5))
    }
}
```

### Integer Square Root

```xfawa
#sqrt {
    fn main() {
        fn isqrt(n) {
            if n < 0 {
                return -1
            }
            if n == 0 {
                return 0
            }
            
            int x = n
            int y = (x + 1) / 2
            
            while y < x {
                x = y
                y = (x + n / x) / 2
            }
            
            return x
        }
        
        print("sqrt(16) = " + isqrt(16))
        print("sqrt(100) = " + isqrt(100))
        print("sqrt(1000) = " + isqrt(1000))
    }
}
```

### GCD and LCM

```xfawa
#gcd_lcm {
    fn main() {
        fn gcd(a, b) {
            while b != 0 {
                int temp = b
                b = a % b
                a = temp
            }
            return a
        }
        
        fn lcm(a, b) {
            return (a * b) / gcd(a, b)
        }
        
        print("GCD(48, 18) = " + gcd(48, 18))
        print("LCM(48, 18) = " + lcm(48, 18))
        print("GCD(100, 35) = " + gcd(100, 35))
        print("LCM(100, 35) = " + lcm(100, 35))
    }
}
```

---

## Advanced Comparison Techniques

### Three-Way Comparison

```xfawa
#three_way_compare {
    fn main() {
        fn compare(a, b) {
            if a < b {
                return -1
            }
            else if a > b {
                return 1
            }
            return 0
        }
        
        fn sign(n) {
            return compare(n, 0)
        }
        
        print("compare(5, 10) = " + compare(5, 10))
        print("compare(10, 5) = " + compare(10, 5))
        print("compare(7, 7) = " + compare(7, 7))
        
        print("sign(-5) = " + sign(-5))
        print("sign(0) = " + sign(0))
        print("sign(5) = " + sign(5))
    }
}
```

### Range Checking

```xfawa
#range_check {
    fn main() {
        fn in_range(value, min, max) {
            return value >= min && value <= max
        }
        
        fn clamp(value, min, max) {
            if value < min {
                return min
            }
            if value > max {
                return max
            }
            return value
        }
        
        fn wrap(value, min, max) {
            int range = max - min + 1
            while value < min {
                value = value + range
            }
            while value > max {
                value = value - range
            }
            return value
        }
        
        print("in_range(5, 1, 10) = " + in_range(5, 1, 10))
        print("clamp(15, 1, 10) = " + clamp(15, 1, 10))
        print("wrap(13, 1, 12) = " + wrap(13, 1, 12))
    }
}
```

---

## Advanced Logical Techniques

### Bit Flags Simulation

```xfawa
#flags {
    fn main() {
        int FLAG_A = 1
        int FLAG_B = 2
        int FLAG_C = 4
        int FLAG_D = 8
        
        fn has_flag(flags, flag) {
            return (flags / flag) % 2 == 1
        }
        
        fn add_flag(flags, flag) {
            if !has_flag(flags, flag) {
                return flags + flag
            }
            return flags
        }
        
        fn remove_flag(flags, flag) {
            if has_flag(flags, flag) {
                return flags - flag
            }
            return flags
        }
        
        fn toggle_flag(flags, flag) {
            if has_flag(flags, flag) {
                return remove_flag(flags, flag)
            }
            return add_flag(flags, flag)
        }
        
        int permissions = 0
        permissions = add_flag(permissions, FLAG_A)
        permissions = add_flag(permissions, FLAG_C)
        
        print("Has A: " + has_flag(permissions, FLAG_A))
        print("Has B: " + has_flag(permissions, FLAG_B))
        print("Has C: " + has_flag(permissions, FLAG_C))
        
        permissions = toggle_flag(permissions, FLAG_B)
        print("After toggle B: " + has_flag(permissions, FLAG_B))
    }
}
```

### Short-Circuit Evaluation Applications

```xfawa
#short_circuit_advanced {
    fn main() {
        fn safe_divide(a, b) {
            if b != 0 && a / b > 0 {
                return a / b
            }
            return 0
        }
        
        fn safe_array_access(arr, index) {
            if index >= 0 && index < len(arr) {
                return arr[index]
            }
            return -1
        }
        
        fn safe_mod(a, b) {
            if b != 0 {
                return a % b
            }
            return 0
        }
        
        print("safe_divide(10, 2) = " + safe_divide(10, 2))
        print("safe_divide(10, 0) = " + safe_divide(10, 0))
        
        int arr = [1, 2, 3, 4, 5]
        print("safe_array_access(arr, 2) = " + safe_array_access(arr, 2))
        print("safe_array_access(arr, 10) = " + safe_array_access(arr, 10))
    }
}
```

### Ternary Operation Simulation

```xfawa
#ternary_simulation {
    fn main() {
        fn select(condition, true_val, false_val) {
            if condition {
                return true_val
            }
            return false_val
        }
        
        fn min(a, b) {
            return select(a < b, a, b)
        }
        
        fn max(a, b) {
            return select(a > b, a, b)
        }
        
        fn abs(n) {
            return select(n < 0, -n, n)
        }
        
        print("min(5, 3) = " + min(5, 3))
        print("max(5, 3) = " + max(5, 3))
        print("abs(-7) = " + abs(-7))
        print("abs(7) = " + abs(7))
    }
}
```

---

## Advanced Modulo Techniques

### Cyclic Indexing

```xfawa
#cyclic_index {
    fn main() {
        fn cyclic_get(arr, index) {
            int len = len(arr)
            int normalized = index % len
            if normalized < 0 {
                normalized = normalized + len
            }
            return arr[normalized]
        }
        
        fn cyclic_next(current, total) {
            return (current + 1) % total
        }
        
        fn cyclic_prev(current, total) {
            return (current - 1 + total) % total
        }
        
        int colors = [1, 2, 3]
        print("colors[0] = " + cyclic_get(colors, 0))
        print("colors[3] = " + cyclic_get(colors, 3))
        print("colors[-1] = " + cyclic_get(colors, -1))
        
        int current = 2
        print("Next of 2: " + cyclic_next(current, 3))
        print("Prev of 2: " + cyclic_prev(current, 3))
    }
}
```

### Digit Extraction

```xfawa
#digit_extraction {
    fn main() {
        fn get_last_digit(n) {
            return abs(n) % 10
        }
        
        fn remove_last_digit(n) {
            return n / 10
        }
        
        fn get_digit(n, position) {
            int i = 0
            while i < position {
                n = n / 10
                i = i + 1
            }
            return abs(n) % 10
        }
        
        fn count_digits(n) {
            if n == 0 {
                return 1
            }
            int count = 0
            n = abs(n)
            while n > 0 {
                count = count + 1
                n = n / 10
            }
            return count
        }
        
        print("Last digit of 12345: " + get_last_digit(12345))
        print("Digit at position 2 of 12345: " + get_digit(12345, 2))
        print("Digit count of 12345: " + count_digits(12345))
    }
}
```

---

## Advanced Range Techniques

### Range Generator

```xfawa
#range_generator {
    fn main() {
        fn range_step(start, end, step) {
            int result = []
            int i = 0
            int current = start
            
            if step > 0 {
                while current <= end {
                    result[i] = current
                    current = current + step
                    i = i + 1
                }
            }
            else if step < 0 {
                while current >= end {
                    result[i] = current
                    current = current + step
                    i = i + 1
                }
            }
            
            return result
        }
        
        int evens = range_step(0, 10, 2)
        int odds = range_step(1, 10, 2)
        int countdown = range_step(10, 1, -1)
        
        print("Evens:")
        for n in evens {
            print(n)
        }
        
        print("Countdown:")
        for n in countdown {
            print(n)
        }
    }
}
```

### Range Operations

```xfawa
#range_operations {
    fn main() {
        fn range_sum(start, end) {
            int total = 0
            int i = start
            while i <= end {
                total = total + i
                i = i + 1
            }
            return total
        }
        
        fn range_product(start, end) {
            int total = 1
            int i = start
            while i <= end {
                total = total * i
                i = i + 1
            }
            return total
        }
        
        fn range_contains(range_arr, value) {
            for n in range_arr {
                if n == value {
                    return true
                }
            }
            return false
        }
        
        print("Sum 1 to 10: " + range_sum(1, 10))
        print("Product 1 to 5: " + range_product(1, 5))
        
        int r = [1...10]
        print("Contains 5: " + range_contains(r, 5))
        print("Contains 15: " + range_contains(r, 15))
    }
}
```

---

## Combined Example

### Math Expression Evaluator

```xfawa
#expression_evaluator {
    fn main() {
        fn evaluate(a, b, op) {
            if op == "+" {
                return a + b
            }
            else if op == "-" {
                return a - b
            }
            else if op == "*" {
                return a * b
            }
            else if op == "/" {
                if b != 0 {
                    return a / b
                }
                return 0
            }
            else if op == "%" {
                if b != 0 {
                    return a % b
                }
                return 0
            }
            return 0
        }
        
        fn compare_all(a, b) {
            print("a = " + a + ", b = " + b)
            print("a == b: " + (a == b))
            print("a != b: " + (a != b))
            print("a < b: " + (a < b))
            print("a <= b: " + (a <= b))
            print("a > b: " + (a > b))
            print("a >= b: " + (a >= b))
        }
        
        print("10 + 5 = " + evaluate(10, 5, "+"))
        print("10 - 5 = " + evaluate(10, 5, "-"))
        print("10 * 5 = " + evaluate(10, 5, "*"))
        print("10 / 5 = " + evaluate(10, 5, "/"))
        print("10 % 3 = " + evaluate(10, 3, "%"))
        
        print("")
        compare_all(10, 20)
    }
}
```

### Calculator Application

```xfawa
#calculator {
    fn main() {
        window {
            width: 350
            height: 450
            title: "Calculator"
            
            box {
                id: display
                x: 25
                y: 20
                width: 300
                height: 60
                text: "0"
            }
            
            int current = 0
            int stored = 0
            string operation = ""
            
            button {
                x: 25
                y: 100
                width: 70
                height: 60
                text: "7"
                current = current * 10 + 7
                print(current, display)
            }
            
            button {
                x: 100
                y: 100
                width: 70
                height: 60
                text: "8"
                current = current * 10 + 8
                print(current, display)
            }
            
            button {
                x: 175
                y: 100
                width: 70
                height: 60
                text: "9"
                current = current * 10 + 9
                print(current, display)
            }
            
            button {
                x: 250
                y: 100
                width: 70
                height: 60
                text: "/"
                stored = current
                current = 0
                operation = "/"
            }
            
            button {
                x: 25
                y: 170
                width: 70
                height: 60
                text: "4"
                current = current * 10 + 4
                print(current, display)
            }
            
            button {
                x: 100
                y: 170
                width: 70
                height: 60
                text: "5"
                current = current * 10 + 5
                print(current, display)
            }
            
            button {
                x: 175
                y: 170
                width: 70
                height: 60
                text: "6"
                current = current * 10 + 6
                print(current, display)
            }
            
            button {
                x: 250
                y: 170
                width: 70
                height: 60
                text: "*"
                stored = current
                current = 0
                operation = "*"
            }
            
            button {
                x: 25
                y: 240
                width: 70
                height: 60
                text: "1"
                current = current * 10 + 1
                print(current, display)
            }
            
            button {
                x: 100
                y: 240
                width: 70
                height: 60
                text: "2"
                current = current * 10 + 2
                print(current, display)
            }
            
            button {
                x: 175
                y: 240
                width: 70
                height: 60
                text: "3"
                current = current * 10 + 3
                print(current, display)
            }
            
            button {
                x: 250
                y: 240
                width: 70
                height: 60
                text: "-"
                stored = current
                current = 0
                operation = "-"
            }
            
            button {
                x: 25
                y: 310
                width: 70
                height: 60
                text: "C"
                current = 0
                stored = 0
                operation = ""
                print("0", display)
            }
            
            button {
                x: 100
                y: 310
                width: 70
                height: 60
                text: "0"
                current = current * 10 + 0
                print(current, display)
            }
            
            button {
                x: 175
                y: 310
                width: 70
                height: 60
                text: "="
                if operation == "+" {
                    current = stored + current
                }
                else if operation == "-" {
                    current = stored - current
                }
                else if operation == "*" {
                    current = stored * current
                }
                else if operation == "/" {
                    if current != 0 {
                        current = stored / current
                    }
                }
                print(current, display)
            }
            
            button {
                x: 250
                y: 310
                width: 70
                height: 60
                text: "+"
                stored = current
                current = 0
                operation = "+"
            }
        }
    }
}
```

---

## Related Documentation

- [Operators](../Operators.md)
- [Data Types Advanced](./DataTypesAdvanced.md)
- [Control Flow Advanced](./ControlFlowAdvanced.md)
