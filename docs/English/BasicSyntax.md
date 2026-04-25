# Basic Syntax

This document introduces the basic syntax of xfawaPL.

---

## Code Block

Code blocks are the basic organizational unit of xfawaPL, defined using the `#` symbol.

### Syntax

```xfawa
#block_name {
    // code content
}
```

### Rules

- Block names must be valid identifiers
- A source file can contain multiple code blocks
- Each code block can contain function definitions

### Example

```xfawa
#MainModule {
    fn main() {
        print("Hello from MainModule")
    }
}

#HelperModule {
    fn helper() {
        print("Helper function")
    }
}
```

---

## Function Definition

Use the `fn` keyword to define functions.

### Syntax

```xfawa
fn function_name(param1, param2) {
    // function body
}
```

### Rules

- Functions must be defined within code blocks
- Two sibling functions cannot be defined in one code block (nested definitions within functions are allowed)
- The `main` function is the program entry point
- Parameter list can be empty

### Example

```xfawa
#app {
    fn main() {
        print("Entry point")
    }
    
    fn greet(name) {
        print(name)
    }
}
```

---

## Comments

Supports single-line and multi-line comments.

### Single-line Comments

```xfawa
// This is a single-line comment
print("Hello") // End-of-line comment
```

### Multi-line Comments

```xfawa
/* 
 * This is a multi-line comment
 * Can span multiple lines
 * Can contain any characters: @#$%^&*()
 */
```

### Example

```xfawa
#example {
    fn main() {
        // This is a single-line comment
        print("Hello") // End-of-line comment
        
        /* 
         * This is a multi-line comment
         * Can contain any characters
         */
    }
}
```

---

## Program Entry

Every xfawaPL program needs a `main` function as the entry point.

### Minimal Program

```xfawa
#hello {
    fn main() {
        print("Hello, World!")
    }
}
```

---

## No Semicolon Design

xfawaPL does not require semicolons at the end of lines.

```xfawa
#example {
    fn main() {
        int a = 1
        int b = 2
        int c = a + b
        print(c)
    }
}
```

---

## Related Documents

- [Basic Syntax Advanced](./advanced/BasicSyntaxAdvanced.md)
- [Data Types](./DataTypes.md)
- [Operators](./Operators.md)
- [Control Flow](./ControlFlow.md)
