# Data Types

This document introduces the data types supported by xfawaPL.

---

## Basic Types

| Type | Keyword | Description | Example |
|------|---------|-------------|---------|
| Integer | `int` | 32-bit signed integer | `int a = 42` |
| Long Integer | `long` | 64-bit signed integer | `long b = 3000000000` |
| Float | `float` | Single-precision floating point | `float c = 3.14` |
| Boolean | `bool` | Boolean type | `bool d = true` |
| String | `string` | String type | `string s = "Hello"` |

---

## Variable Declaration

### Explicit Type Declaration (Recommended)

```xfawa
int a = 1
long b = 3000000000
float c = 3.14
bool d = true
string s = "Hello, xfawaPL!"
```

### Implicit Type Declaration

```xfawa
a = 1          // Will produce a compiler warning, explicit type declaration recommended
```

---

## Literals

### Number Literals

```xfawa
int num = 42
int max = 2147483647        // Maximum integer value
long bigNum = 3000000000    // Integer exceeding 32-bit uses long
long maxLong = 9223372036854775807  // Maximum long value
float pi = 3.14159
float scientific = 1.5e10   // Scientific notation
```

### Integer Range

- Integer (int) literal valid range: `-2147483648` to `2147483647`
- Long integer (long) literal valid range: `-9223372036854775808` to `9223372036854775807`
- Negative numbers are achieved through unary minus: `int neg = -42`

### Boolean Literals

```xfawa
bool yes = true
bool no = false
```

### String Literals

```xfawa
string text = "Hello, xfawaPL!"
string empty = ""
string multiline = "Line1\nLine2"    // Supports escape characters
```

#### Escape Characters

| Escape Sequence | Description |
|-----------------|-------------|
| `\n` | Newline |
| `\t` | Tab |
| `\r` | Carriage return |
| `\\` | Backslash |
| `\"` | Double quote |
| `\0` | Null character |

---

## Arrays

### Array Definition

#### Explicit Element List

```xfawa
int arr1 = [1, 2, 3, 4, 5]
int arr2 = [10, 20, 30]
```

#### Range Syntax

```xfawa
int arr1 = [1...10]      // Array from 1 to 10
int arr2 = [0...100]     // Array from 0 to 100
```

### Array Indexing

```xfawa
int arr = [1, 2, 3, 4, 5]

print(arr[0])      // Access first element, output: 1
print(arr[2])      // Access third element, output: 3
print(arr[-1])     // Access last element, output: 5
print(arr[-2])     // Access second-to-last element, output: 4
```

### Array Range

```xfawa
int arr = [1...10]
int sub = arr[2...5]    // Get subarray from index 2 to 5
```

### Array Traversal

```xfawa
int arr = [1, 2, 3, 4, 5]

for item in arr {
    print(item)
}
```

---

## Complete Example

```xfawa
#types_demo {
    fn main() {
        int integer = 42
        float decimal = 3.14159
        bool flag = true
        
        print("Integer:")
        print(integer)
        
        print("Float:")
        print(decimal)
        
        print("Boolean:")
        print(flag)
        
        int arr = [1, 2, 3, 4, 5]
        print("Array first element:")
        print(arr[0])
        
        int range = [1...10]
        print("Range array random element:")
        print(rnd(range))
    }
}
```

---

## Related Documents

- [Data Types Advanced](./advanced/DataTypesAdvanced.md)
- [Basic Syntax](./BasicSyntax.md)
- [Operators](./Operators.md)
- [Built-in Functions](./Builtins.md)
