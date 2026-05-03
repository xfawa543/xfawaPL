# Basic Syntax Advanced

This document introduces advanced techniques and combined usage of xfawaPL basic syntax.

---

## Advanced Code Block Usage

### Code Block Nesting and Modular Design

xfawaPL supports multiple code blocks working together to achieve modular program design.

```xfawa
#math_utils {
    fn add(a, b) {
        print(a + b)
    }
    
    fn subtract(a, b) {
        print(a - b)
    }
    
    fn multiply(a, b) {
        print(a * b)
    }
}

#app {
    fn main() {
        print("=== Math Utils ===")
        math_utils:add(10, 20)
        math_utils:subtract(50, 15)
        math_utils:multiply(6, 7)
    }
}
```

### Dependency Management Between Code Blocks

```xfawa
#config {
    fn get_app_name() {
        print("MyApp")
    }
    
    fn get_version() {
        print("1.0.0")
    }
}

#logger {
    fn log(message) {
        print("[LOG] ")
        print(message)
    }
    
    fn error(message) {
        print("[ERROR] ")
        print(message)
    }
    
    fn info(message) {
        print("[INFO] ")
        print(message)
    }
}

#app {
    fn main() {
        print("App: ")
        config:get_app_name()
        print("Version: ")
        config:get_version()
        
        logger:info("Application started")
        logger:error("Something went wrong")
    }
}
```

---

## Advanced Function Usage

### Function Parameters and Calls

xfawaPL functions support parameter passing for code reuse.

```xfawa
#function_demo {
    fn greet(name) {
        print("Hello, ")
        print(name)
    }
    
    fn calculate(a, b, c) {
        print("Sum: ")
        print(a + b + c)
        print("Product: ")
        print(a * b * c)
    }
    
    fn main() {
        greet("Alice")
        greet("Bob")
        calculate(2, 3, 4)
    }
}
```

### Multi-Code Block Function Collaboration

```xfawa
#validator {
    fn check_positive(n) {
        if n > 0 {
            print(1)
        }
        else {
            print(0)
        }
    }
    
    fn check_range(n, min, max) {
        if n >= min {
            if n <= max {
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

#app {
    fn main() {
        print("=== Validator Demo ===")
        
        print("Is 5 positive?")
        validator:check_positive(5)
        
        print("Is -3 positive?")
        validator:check_positive(-3)
        
        print("Is 15 in range [10, 20]?")
        validator:check_range(15, 10, 20)
        
        print("Is 25 in range [10, 20]?")
        validator:check_range(25, 10, 20)
    }
}
```

---

## Advanced Comment Usage

### Documentation Comment Style

```xfawa
#documented_code {
    fn main() {
        /*
         * ========================================
         * User Management System
         * Version: 1.0.0
         * Author: xfawaPL Team
         * ========================================
         */
        
        // Initialize user data
        int user_count = 0
        
        // TODO: Add user validation
        // FIXME: Age range check needs optimization
        
        print("User system initialized")
    }
}
```

---

## Program Structure Patterns

### Master-Slave Module Pattern

```xfawa
#main_controller {
    fn main() {
        print("=== Application Start ===")
        
        data_handler:init()
        business_logic:process()
        ui_renderer:display()
        
        print("=== Application End ===")
    }
}

#data_handler {
    fn init() {
        print("Initializing data...")
    }
    
    fn load_data() {
        print("Loading data...")
    }
}

#business_logic {
    fn process() {
        data_handler:load_data()
        print("Processing data...")
    }
}

#ui_renderer {
    fn display() {
        print("Rendering UI...")
    }
}
```

### State Machine Pattern

```xfawa
#state_machine {
    fn main() {
        int state = 0
        int running = 1
        
        while (running == 1) {
            if state == 0 {
                print("State: IDLE")
                state = 1
            }
            else if state == 1 {
                print("State: LOADING")
                state = 2
            }
            else if state == 2 {
                print("State: PROCESSING")
                state = 3
            }
            else if state == 3 {
                print("State: COMPLETE")
                running = 0
            }
        }
    }
}
```

---

## Practical Design Patterns

### Counter Pattern

```xfawa
#counter {
    fn increment(current) {
        print(current + 1)
    }
    
    fn decrement(current) {
        print(current - 1)
    }
    
    fn add(current, amount) {
        print(current + amount)
    }
}

#app {
    fn main() {
        print("=== Counter Demo ===")
        
        print("Increment 5:")
        counter:increment(5)
        
        print("Decrement 10:")
        counter:decrement(10)
        
        print("Add 7 to 15:")
        counter:add(15, 7)
    }
}
```

### Factory Pattern Simulation

```xfawa
#factory {
    fn create_button() {
        print("Button created")
    }
    
    fn create_text() {
        print("Text created")
    }
    
    fn create_box() {
        print("Box created")
    }
    
    fn create(type_id) {
        if type_id == 1 {
            create_button()
        }
        else if type_id == 2 {
            create_text()
        }
        else if type_id == 3 {
            create_box()
        }
        else {
            print("Unknown type")
        }
    }
}

#app {
    fn main() {
        print("=== Factory Demo ===")
        
        factory:create(1)
        factory:create(2)
        factory:create(3)
    }
}
```

---

## Combined Examples

### Complete Application Structure

```xfawa
#config {
    fn get_title() {
        print("My Application")
    }
    
    fn get_width() {
        print(400)
    }
    
    fn get_height() {
        print(300)
    }
}

#utils {
    fn format_number(n) {
        print("Number: ")
        print(n)
    }
    
    fn clamp(value, min, max) {
        int result = value
        if value < min {
            result = min
        }
        else if value > max {
            result = max
        }
        print(result)
    }
}

#app {
    fn main() {
        print("=== App Config ===")
        
        print("Title: ")
        config:get_title()
        
        print("Width: ")
        config:get_width()
        
        print("Height: ")
        config:get_height()
        
        print("")
        print("=== Utils Demo ===")
        
        print("Format 42:")
        utils:format_number(42)
        
        print("Clamp 150 to [0, 100]:")
        utils:clamp(150, 0, 100)
    }
}
```

---

## Related Documentation

- [Basic Syntax](../BasicSyntax.md)
- [Control Flow Advanced](./ControlFlowAdvanced.md)
- [Window System Advanced](./WindowSystemAdvanced.md)
