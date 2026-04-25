# Mods System

This document provides a detailed introduction to xfawaPL's Mods modular extension system.

---

## Overview

The Mods system is xfawaPL's modular extension mechanism that allows developers to:

- **Modify Keywords**: Replace language keywords with custom names
- **Add Custom Syntax**: Create entirely new syntax structures
- **Export Public Functions**: Define reusable function libraries

---

## Mod Files

### File Format

- **Extension**: `.xfmod`
- **Examples**: `prob.xfmod`, `utils.xfmod`, `math.xfmod`

### File Location

Mod files are typically stored in the project's `mods/` directory, where the compiler automatically searches during the preprocessing stage.

### Basic Structure

```xfawa
#mod_name {
    fn main() {
        // Syntax modification
        "old_keyword" => "new_keyword"
        
        // Public function definition
        fn namespace:functionName() {
            // ...
        }
        
        // Internal function definition
        fn internalFunc() {
            // ...
        }
    }
    
    // Syntax addition (inside block, outside function)
    @add {
        name: syntaxName
        syntax: ...
        logic: {
            // ...
        }
    }
}
```

---

## Importing Mods

### Import Method

**All mod imports must use `%import`**, this is the only import method:

```xfawa
// Method 1: Use filename (recommended)
%import "prob"           // Automatically adds .xfmod suffix

// Method 2: Use complete filename
%import "prob.xfmod"
```

### Important Notes

1. **Must use `%import` only**: Whether adding new syntax mods or exporting function mods, all must use `%import`
2. **Import statements cannot be modified**: `%import` is a preprocessor directive and cannot be changed through syntax modification
3. **Executed at preprocessing stage**: `%import` executes during compilation preprocessing, loading mods before syntax analysis
4. **Position requirement**: Import statements must be placed at the top of the file, before module definitions

### Import Rules

1. Mod files must exist in a path accessible to the compiler
2. Filenames are case-sensitive
3. Multiple mods are loaded in import order

### Examples

```xfawa
// Import single mod
%import "prob"

#app {
    fn main() {
        prob 30 50 {
            print("Triggered!")
        }
    }
}
```

```xfawa
// Import multiple mods
%import "prob"
%import "repeat"
%import "utils"

#app {
    fn main() {
        // Use functionality from multiple mods
    }
}
```

---

## Syntax Modification

Replace xfawaPL keywords with custom names.

### Syntax Format

```xfawa
"original_keyword" => "new_keyword"
```

### Modifiable Keywords

| Keyword | Description |
|---------|-------------|
| `fn` | Function definition |
| `if` | Conditional |
| `else` | Else branch |
| `while` | Loop |
| `break` | Break loop |
| `return` | Return |
| `true` | Boolean true |
| `false` | Boolean false |
| `print` | Print output |

### Example

Create `keywords.xfmod`:

```xfawa
#keywords {
    fn main() {
        "print" => "say"
        "if" => "when"
        "else" => "otherwise"
    }
}
```

Use modified syntax:

```xfawa
%import "keywords"

#app {
    fn main() {
        when true {
            say("True!")
        }
        otherwise {
            say("False!")
        }
    }
}
```

### Conflict Handling

When multiple mods attempt to modify the same keyword, a conflict occurs and the compiler reports an error.

---

## Syntax Addition (@add)

Use the `@add` directive to add entirely new syntax structures.

### Syntax Format

```xfawa
@add {
    name: <syntax_name>
    syntax: <syntax_pattern>
    logic: {
        <logic_implementation>
    }
}
```

### Parameter Placeholders

| Placeholder | Description |
|-------------|-------------|
| `<param_name>` | Regular parameter |
| `<action>` | Code block parameter |

### Example: Probability Execution Syntax

Create `prob.xfmod`:

```xfawa
#prob_syntax {
    @add {
        name: prob
        syntax: prob <min_value> <max_value> { <action> }
        logic: {
            int prob_p = rnd(<min_value>, <max_value>)
            int prob_r = rnd(1, 100)
            if prob_r <= prob_p {
                <action>
            }
        }
    }
}
```

Usage:

```xfawa
%import "prob"

#test {
    fn main() {
        prob 30 50 {
            print("30-50% chance triggered!")
        }
    }
}
```

### Limitations

- Maximum 256 custom syntaxes per mod
- Syntax names must be unique

---

## Public Function Export

Define functions that can be called externally.

### Syntax Format

```xfawa
fn <namespace>:<function_name>() {
    // function body
}
```

### Example

Create `utils.xfmod`:

```xfawa
#utils {
    fn main() {
        fn pub:greet(name) {
            print("Hello, " + name + "!")
        }
        
        fn pub:add(a, b) {
            return a + b
        }
    }
}
```

Usage:

```xfawa
%import "utils"

#app {
    fn main() {
        pub:greet("World")
        
        int result = pub:add(10, 20)
        print(result)
    }
}
```

---

## Internal Functions

Functions without namespace are only visible within the mod.

### Example

```xfawa
#pub_with_internal {
    fn main() {
        fn pub:publicFunc() {
            print("Public function")
            helper()
        }
        
        fn helper() {
            print("Internal helper")
        }
    }
}
```

---

## Namespaces

### Reserved Namespaces

The following namespaces are reserved by the system and cannot be used:

| Namespace | Purpose |
|-----------|---------|
| `core` | Core functionality |
| `sys` | System functionality |
| `runtime` | Runtime functionality |
| `compiler` | Compiler functionality |
| `internal` | Internal functionality |
| `builtin` | Built-in functionality |
| `std` | Standard library |
| `ext` | Extension functionality |
| `lib` | Library functionality |

### Namespace Rules

- Cannot use reserved namespaces
- Cannot use multi-level namespaces (e.g., `pub:xxx:xxx`)

### Valid Examples

```xfawa
fn pub:myFunction() {}      // Valid
fn mylib:helper() {}        // Valid
fn utils:process() {}       // Valid
```

### Invalid Examples

```xfawa
fn core:test() {}           // Error: core is a reserved namespace
fn pub:sub:func() {}        // Error: multi-level namespace
```

---

## Related Documents

- [Mods Advanced Usage](./advanced/ModsAdvanced.md)
- [Basic Syntax](./BasicSyntax.md)
- [Built-in Functions](./Builtins.md)
- [Window System](./WindowSystem.md)
