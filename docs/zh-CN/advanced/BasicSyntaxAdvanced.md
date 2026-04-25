# 基础语法高阶用法 (Basic Syntax Advanced)

本文档介绍 xfawaPL 基础语法的高级技巧和组合用法。

---

## 代码块高级用法

### 代码块嵌套与模块化设计

xfawaPL 支持多个代码块协同工作，实现模块化程序设计。

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

### 代码块间的依赖管理

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

## 函数高级用法

### 函数嵌套与作用域

xfawaPL 支持在函数内部定义嵌套函数，实现局部作用域。

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

### 递归函数

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

### 高阶函数模式

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

## 注释高级用法

### 文档注释风格

```xfawa
#documented_code {
    fn main() {
        /*
         * ========================================
         * 用户管理系统
         * 版本: 1.0.0
         * 作者: xfawaPL Team
         * ========================================
         */
        
        fn create_user(name, age) {
            /*
             * 创建新用户
             * 参数:
             *   name - 用户名称
             *   age - 用户年龄
             * 返回:
             *   用户信息字符串
             */
            return "User: " + name + ", Age: " + age
        }
        
        // TODO: 添加用户验证功能
        // FIXME: 年龄范围检查需要优化
        
        string user = create_user("Alice", 25)
        print(user)
    }
}
```

---

## 程序结构模式

### 主从模块模式

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

### 状态机模式

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

## 实用设计模式

### 单例模式模拟

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

### 工厂模式模拟

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

## 组合示例

### 完整应用程序结构

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

## 相关文档

- [基础语法](../BasicSyntax.md)
- [控制流高阶用法](./ControlFlowAdvanced.md)
- [窗口系统高阶用法](./WindowSystemAdvanced.md)
