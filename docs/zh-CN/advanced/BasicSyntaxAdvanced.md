# 基础语法高阶用法 (Basic Syntax Advanced)

本文档介绍 xfawaPL 基础语法的高级技巧和组合用法。

---

## 代码块高级用法

### 代码块嵌套与模块化设计

xfawaPL 支持多个代码块协同工作，实现模块化程序设计。

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

### 代码块间的依赖管理

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

## 函数高级用法

### 函数参数与调用

xfawaPL 函数支持参数传递，实现代码复用。

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

### 多代码块函数协作

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
        
        // 初始化用户数据
        int user_count = 0
        
        // TODO: 添加用户验证功能
        // FIXME: 年龄范围检查需要优化
        
        print("User system initialized")
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

### 状态机模式

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

## 实用设计模式

### 计数器模式

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

### 工厂模式模拟

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

## 组合示例

### 完整应用程序结构

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

## 相关文档

- [基础语法](../BasicSyntax.md)
- [控制流高阶用法](./ControlFlowAdvanced.md)
- [窗口系统高阶用法](./WindowSystemAdvanced.md)
