# Control Flow

This document introduces control flow statements in xfawaPL.

---

## Conditional Statements (if/else)

### Basic if Statement

```xfawa
if condition {
    // Execute when condition is true
}
```

### if-else Statement

```xfawa
if condition {
    // Execute when condition is true
}
else {
    // Execute when condition is false
}
```

### if-else if-else Chain

```xfawa
int score = 75

if score >= 90 {
    print("Excellent")
}
else if score >= 80 {
    print("Good")
}
else if score >= 60 {
    print("Pass")
}
else {
    print("Fail")
}
```

### Nested Conditions

```xfawa
int a = 10
int b = 20

if a > 0 {
    if b > 0 {
        print("Both a and b are positive")
    }
    else {
        print("a is positive, b is not")
    }
}
else {
    print("a is not positive")
}
```

### Complex Conditions

```xfawa
int age = 25
bool hasLicense = true

if age >= 18 && hasLicense {
    print("Can drive")
}
else {
    print("Cannot drive")
}
```

---

## Loop Statements

### while Loop

```xfawa
int i = 0
while (i < 10) {
    print(i)
    i = i + 1
}
```

#### while Loop Example

```xfawa
#while_demo {
    fn main() {
        int count = 0
        int sum = 0
        
        while (count <= 100) {
            sum = sum + count
            count = count + 1
        }
        
        print("Sum of 1 to 100:")
        print(sum)
    }
}
```

### for-in Loop

#### Iterate Array

```xfawa
int arr = [1, 2, 3, 4, 5]
for item in arr {
    print(item)
}
```

#### Iterate Range

```xfawa
for i in [1...10] {
    print(i)
}
```

#### for-in Loop Example

```xfawa
#for_demo {
    fn main() {
        int numbers = [10, 20, 30, 40, 50]
        
        print("Iterate array:")
        for num in numbers {
            print(num)
        }
        
        print("Iterate range:")
        for i in [1...5] {
            print(i)
        }
    }
}
```

---

## break Statement

`break` is used to exit a loop.

### Example

```xfawa
#break_demo {
    fn main() {
        int i = 0
        while (i < 100) {
            if i == 5 {
                break    // Exit loop when i equals 5
            }
            print(i)
            i = i + 1
        }
        print("Loop ended")
    }
}
```

### Using break in for-in

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

## return Statement

`return` is used to return a value from a function.

### No Return Value

```xfawa
fn greet(name) {
    print("Hello, " + name)
    return
}
```

### With Return Value

```xfawa
fn add(a, b) {
    return a + b
}

fn multiply(a, b) {
    return a * b
}
```

### Example

```xfawa
#return_demo {
    fn main() {
        int result = add(10, 20)
        print(result)    // Output: 30
        
        int product = multiply(5, 6)
        print(product)   // Output: 30
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

## Related Documents

- [Control Flow Advanced Usage](./advanced/ControlFlowAdvanced.md)
- [Operators](./Operators.md)
- [Built-in Functions](./Builtins.md)
