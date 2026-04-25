# Operators

This document introduces the operators supported by xfawaPL.

---

## Arithmetic Operators

| Operator | Description | Example | Result |
|----------|-------------|---------|--------|
| `+` | Addition | `5 + 3` | `8` |
| `-` | Subtraction | `5 - 3` | `2` |
| `*` | Multiplication | `5 * 3` | `15` |
| `/` | Division | `6 / 2` | `3` |
| `%` | Modulo | `7 % 3` | `1` |

### Example

```xfawa
#arithmetic {
    fn main() {
        int a = 10
        int b = 3
        
        print(a + b)    // Output: 13
        print(a - b)    // Output: 7
        print(a * b)    // Output: 30
        print(a / b)    // Output: 3
        print(a % b)    // Output: 1
    }
}
```

---

## Comparison Operators

| Operator | Description | Example | Result |
|----------|-------------|---------|--------|
| `==` | Equal | `5 == 5` | `true` |
| `!=` | Not equal | `5 != 3` | `true` |
| `<` | Less than | `3 < 5` | `true` |
| `<=` | Less than or equal | `3 <= 3` | `true` |
| `>` | Greater than | `5 > 3` | `true` |
| `>=` | Greater than or equal | `5 >= 5` | `true` |

### Example

```xfawa
#comparison {
    fn main() {
        int a = 10
        int b = 20
        
        print(a == b)   // Output: false
        print(a != b)   // Output: true
        print(a < b)    // Output: true
        print(a <= b)   // Output: true
        print(a > b)    // Output: false
        print(a >= b)   // Output: false
    }
}
```

---

## Logical Operators

| Operator | Description | Example | Result |
|----------|-------------|---------|--------|
| `&&` | Logical AND | `true && false` | `false` |
| `\|\|` | Logical OR | `true \|\| false` | `true` |
| `!` | Logical NOT | `!true` | `false` |

### Example

```xfawa
#logical {
    fn main() {
        bool a = true
        bool b = false
        
        print(a && b)   // Output: false
        print(a || b)   // Output: true
        print(!a)       // Output: false
        print(!b)       // Output: true
    }
}
```

---

## Range Operator

| Operator | Description | Example |
|----------|-------------|---------|
| `...` | Range | `[1...10]` |

### Example

```xfawa
#range {
    fn main() {
        int arr = [1...10]      // Create array from 1 to 10
        for i in arr {
            print(i)
        }
    }
}
```

---

## Operator Precedence

From highest to lowest:

1. `!` (Logical NOT)
2. `*`, `/`, `%` (Multiplication, Division, Modulo)
3. `+`, `-` (Addition, Subtraction)
4. `<`, `<=`, `>`, `>=` (Comparison)
5. `==`, `!=` (Equality)
6. `&&` (Logical AND)
7. `||` (Logical OR)
8. `=` (Assignment)

---

## Related Documents

- [Operators Advanced](./advanced/OperatorsAdvanced.md)
- [Data Types](./DataTypes.md)
- [Control Flow](./ControlFlow.md)
