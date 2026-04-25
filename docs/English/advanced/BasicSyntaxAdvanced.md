# Basic Syntax Advanced

This document covers advanced techniques and combined usage of xfawaPL basic syntax.

---

## Advanced Block Usage

### Block Nesting and Modular Design

xfawaPL supports multiple blocks working together for modular program design.

```xfawa
#math_utils {
    fn main() {
        fn pub:add(a, b) {
            return a + b
        }
        
        fn pub:subtract(a, b) {
            return a - b
        }
        
        fn pub:multiply(a, b) {
            return a * b
        }
        
        fn pub:divide(a, b) {
            if b != 0 {
                return a / b
            }
            return 0
        }
    }
}

#string_utils {
    fn main() {
        fn pub:repeat(str, times) {
            string result = ""
            int i = 0
            while i < times {
                result = result + str
                i = i + 1
            }
            return result
        }
        
        fn pub:pad_left(str, length) {
            int diff = length - len(str)
            if diff > 0 {
                return repeat(" ", diff) + str
            }
            return str
        }
    }
}

#app {
    fn main() {
        int sum = math_utils:add(10, 20)
        print("Sum: " + sum)
        
        string padded = string_utils:pad_left("Hello", 10)
        print("Padded: " + padded)
    }
}
```

### Dependency Management Between Blocks

```xfawa
#config {
    fn main() {
        fn pub:get_app_name() {
            return "MyApp"
        }
        
        fn pub:get_version() {
            return "1.0.0"
        }
        
        fn pub:get_max_retries() {
            return 3
        }
    }
}

#logger {
    fn main() {
        fn pub:log(message) {
            string app = config:get_app_name()
            print("[" + app + "] " + message)
        }
        
        fn pub:error(message) {
            log("ERROR: " + message)
        }
        
        fn pub:info(message) {
            log("INFO: " + message)
        }
    }
}

#app {
    fn main() {
        logger:info("Application started")
        logger:error("Something went wrong")
    }
}
```

---

## Advanced Function Usage

### Function Nesting and Scope

xfawaPL supports nested function definitions for local scope.

```xfawa
#nested_functions {
    fn main() {
        fn outer(x) {
            print("Outer: " + x)
            
            fn inner(y) {
                print("Inner: " + y)
                return x + y
            }
            
            return inner(x * 2)
        }
        
        int result = outer(5)
        print("Result: " + result)
    }
}
```

### Recursive Functions

```xfawa
#recursion {
    fn main() {
        fn factorial(n) {
            if n <= 1 {
                return 1
            }
            return n * factorial(n - 1)
        }
        
        fn fibonacci(n) {
            if n <= 1 {
                return n
            }
            return fibonacci(n - 1) + fibonacci(n - 2)
        }
        
        fn gcd(a, b) {
            if b == 0 {
                return a
            }
            return gcd(b, a % b)
        }
        
        print("5! = " + factorial(5))
        print("Fib(10) = " + fibonacci(10))
        print("GCD(48, 18) = " + gcd(48, 18))
    }
}
```

### Higher-Order Function Pattern

```xfawa
#higher_order {
    fn main() {
        fn apply_operation(arr, operation_name) {
            int result = 0
            int i = 0
            
            if operation_name == "sum" {
                while i < len(arr) {
                    result = result + arr[i]
                    i = i + 1
                }
            }
            else if operation_name == "product" {
                result = 1
                while i < len(arr) {
                    result = result * arr[i]
                    i = i + 1
                }
            }
            else if operation_name == "max" {
                result = arr[0]
                while i < len(arr) {
                    if arr[i] > result {
                        result = arr[i]
                    }
                    i = i + 1
                }
            }
            
            return result
        }
        
        int numbers = [1, 2, 3, 4, 5]
        print("Sum: " + apply_operation(numbers, "sum"))
        print("Product: " + apply_operation(numbers, "product"))
        print("Max: " + apply_operation(numbers, "max"))
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
        
        fn create_user(name, age) {
            /*
             * Create a new user
             * Parameters:
             *   name - User name
             *   age - User age
             * Returns:
             *   User info string
             */
            return "User: " + name + ", Age: " + age
        }
        
        // TODO: Add user validation
        // FIXME: Age range check needs optimization
        
        string user = create_user("Alice", 25)
        print(user)
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
    fn main() {
        fn pub:init() {
            print("Initializing data...")
        }
        
        fn pub:load_data() {
            return [1, 2, 3, 4, 5]
        }
    }
}

#business_logic {
    fn main() {
        fn pub:process() {
            int data = data_handler:load_data()
            print("Processing " + len(data) + " items")
        }
    }
}

#ui_renderer {
    fn main() {
        fn pub:display() {
            print("Rendering UI...")
        }
    }
}
```

### State Machine Pattern

```xfawa
#state_machine {
    fn main() {
        int state = 0
        int running = 1
        
        while running == 1 {
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

### Singleton Pattern Simulation

```xfawa
#singleton_example {
    fn main() {
        fn pub:get_instance() {
            return "GlobalInstance"
        }
        
        fn pub:do_something() {
            print("Singleton doing something")
        }
    }
}

#app {
    fn main() {
        singleton_example:do_something()
    }
}
```

### Factory Pattern Simulation

```xfawa
#factory {
    fn main() {
        fn pub:create(type) {
            if type == "button" {
                return "Button created"
            }
            else if type == "text" {
                return "Text created"
            }
            else if type == "box" {
                return "Box created"
            }
            return "Unknown type"
        }
    }
}

#app {
    fn main() {
        string btn = factory:create("button")
        string txt = factory:create("text")
        print(btn)
        print(txt)
    }
}
```

---

## Combined Example

### Complete Application Structure

```xfawa
#config {
    fn main() {
        fn pub:get_title() {
            return "My Application"
        }
        
        fn pub:get_width() {
            return 400
        }
        
        fn pub:get_height() {
            return 300
        }
    }
}

#utils {
    fn main() {
        fn pub:format_number(n) {
            return "Number: " + n
        }
        
        fn pub:clamp(value, min, max) {
            if value < min {
                return min
            }
            if value > max {
                return max
            }
            return value
        }
    }
}

#app {
    fn main() {
        string title = config:get_title()
        int width = config:get_width()
        int height = config:get_height()
        
        window {
            width: width
            height: height
            title: title
            
            text {
                x: 100
                y: 50
                text: title
            }
            
            box {
                id: display
                x: 50
                y: 100
                width: 300
                height: 50
                text: utils:format_number(42)
            }
            
            button {
                x: 50
                y: 170
                width: 300
                height: 40
                text: "Click Me"
                
                int val = rnd(1, 100)
                int clamped = utils:clamp(val, 10, 90)
                print(utils:format_number(clamped), display)
            }
        }
    }
}
```

---

## Related Documentation

- [Basic Syntax](../BasicSyntax.md)
- [Control Flow Advanced](./ControlFlowAdvanced.md)
- [Window System Advanced](./WindowSystemAdvanced.md)
