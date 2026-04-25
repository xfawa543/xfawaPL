# Mods 高阶用法

本文档介绍 Mods 系统的高级用法和组合技巧。

---

## 1. 多个 Mod 组合使用

```xfawa
%import "prob"
%import "repeat"
%import "utils"

#app {
    fn main() {
        // 使用 prob mod 的语法
        prob 30 50 {
            print("Triggered!")
        }
        
        // 使用 repeat mod 的语法
        repeat 5 {
            print("Repeat!")
        }
        
        // 使用 utils mod 的函数
        pub:greet("World")
    }
}
```

---

## 2. 语法修改 + 语法添加组合

创建 `mylang.xfmod`：

```xfawa
#mylang {
    fn main() {
        // 修改关键字
        "print" => "say"
        "if" => "when"
        "else" => "otherwise"
    }
    
    // 添加新语法
    @add {
        name: times
        syntax: times <count> { <action> }
        logic: {
            int times_i = 0
            while (times_i < <count>) {
                <action>
                times_i = times_i + 1
            }
        }
    }
    
    @add {
        name: chance
        syntax: chance <percent>% { <action> }
        logic: {
            int _chance_val = rnd(1, 100)
            if _chance_val <= <percent> {
                <action>
            }
        }
    }
}
```

使用：

```xfawa
%import "mylang"

#app {
    fn main() {
        // 使用修改后的关键字
        say("Hello!")
        
        when true {
            say("True!")
        }
        otherwise {
            say("False!")
        }
        
        // 使用添加的新语法
        times 3 {
            say("Repeated!")
        }
        
        chance 50% {
            say("50% chance!")
        }
    }
}
```

---

## 3. 创建数学函数库

创建 `math.xfmod`：

```xfawa
#math {
    fn main() {
        fn math:square(x) {
            return x * x
        }
        
        fn math:cube(x) {
            return x * x * x
        }
        
        fn math:power(base, exp) {
            int result = 1
            int i = 0
            while (i < exp) {
                result = result * base
                i = i + 1
            }
            return result
        }
        
        fn math:max(a, b) {
            if a > b {
                return a
            }
            return b
        }
        
        fn math:min(a, b) {
            if a < b {
                return a
            }
            return b
        }
        
        fn math:abs(x) {
            if x < 0 {
                return 0 - x
            }
            return x
        }
        
        fn math:factorial(n) {
            if n <= 1 {
                return 1
            }
            int result = 1
            int i = 2
            while (i <= n) {
                result = result * i
                i = i + 1
            }
            return result
        }
    }
}
```

使用：

```xfawa
%import "math"

#app {
    fn main() {
        print("Square of 5:")
        print(math:square(5))
        
        print("Cube of 3:")
        print(math:cube(3))
        
        print("2 to the power of 10:")
        print(math:power(2, 10))
        
        print("Max of 10 and 20:")
        print(math:max(10, 20))
        
        print("Factorial of 5:")
        print(math:factorial(5))
    }
}
```

---

## 4. 创建游戏工具库

创建 `game.xfmod`：

```xfawa
#game {
    fn main() {
        fn game:rollDice() {
            return rnd(1, 6)
        }
        
        fn game:rollMultiple(count) {
            int total = 0
            int i = 0
            while (i < count) {
                total = total + rnd(1, 6)
                i = i + 1
            }
            return total
        }
        
        fn game:flipCoin() {
            return rnd(0, 1)
        }
        
        fn game:coinResult() {
            int result = rnd(0, 1)
            if result == 0 {
                print("Heads")
            }
            else {
                print("Tails")
            }
        }
        
        fn game:randomRange(min, max) {
            return rnd(min, max)
        }
    }
}
```

使用：

```xfawa
%import "game"

#app {
    fn main() {
        window {
            width: 400
            height: 350
            title: "Game Tools"
            
            box {
                id: result
                x: 150
                y: 50
                width: 100
                height: 50
                text: "Result"
            }
            
            button {
                x: 100
                y: 120
                width: 200
                height: 40
                text: "Roll Dice"
                
                int dice = game:rollDice()
                print(dice, result)
            }
            
            button {
                x: 100
                y: 180
                width: 200
                height: 40
                text: "Roll 2 Dice"
                
                int total = game:rollMultiple(2)
                print(total, result)
            }
            
            button {
                x: 100
                y: 240
                width: 200
                height: 40
                text: "Flip Coin"
                
                game:coinResult()
            }
        }
    }
}
```

---

## 5. 复杂自定义语法

创建 `advanced.xfmod`：

```xfawa
#advanced {
    @add {
        name: loop
        syntax: loop <var> from <start> to <end> { <action> }
        logic: {
            int <var> = <start>
            while (<var> <= <end>) {
                <action>
                <var> = <var> + 1
            }
        }
    }
    
    @add {
        name: countdown
        syntax: countdown <start> { <action> }
        logic: {
            int countdown_i = <start>
            while (countdown_i > 0) {
                print(countdown_i)
                countdown_i = countdown_i - 1
            }
            <action>
        }
    }
    
    @add {
        name: unless
        syntax: unless <condition> { <action> }
        logic: {
            if !<condition> {
                <action>
            }
        }
    }
    
    @add {
        name: when
        syntax: when <condition> do <action>
        logic: {
            if <condition> {
                <action>
            }
        }
    }
}
```

使用：

```xfawa
%import "advanced"

#app {
    fn main() {
        // loop 语法
        loop i from 1 to 5 {
            print(i)
        }
        
        // countdown 语法
        countdown 3 {
            print("Go!")
        }
        
        // unless 语法
        int x = 5
        unless x == 10 {
            print("x is not 10")
        }
        
        // when 语法
        when x == 5 do print("x is 5")
    }
}
```

---

## 6. 创建调试工具 Mod

创建 `debug.xfmod`：

```xfawa
#debug {
    fn main() {
        fn debug:log(message) {
            print("[DEBUG] " + message)
        }
        
        fn debug:warn(message) {
            print("[WARN] " + message)
        }
        
        fn debug:error(message) {
            print("[ERROR] " + message)
        }
        
        fn debug:assert(condition, message) {
            if !condition {
                print("[ASSERT FAILED] " + message)
            }
        }
        
        fn debug:trace(varName, value) {
            print("[TRACE] " + varName + " = " + value)
        }
    }
    
    @add {
        name: debug_print
        syntax: debug_print <expr>
        logic: {
            print("[DEBUG] " + <expr>)
        }
    }
    
    @add {
        name: assert
        syntax: assert <condition> with <message>
        logic: {
            if !<condition> {
                print("[ASSERT FAILED] " + <message>)
            }
        }
    }
}
```

使用：

```xfawa
%import "debug"

#app {
    fn main() {
        int x = 10
        int y = 20
        
        debug:log("Starting program")
        debug:trace("x", x)
        
        assert x > 0 with "x must be positive"
        
        debug_print x + y
        
        debug:assert(x < y, "x should be less than y")
    }
}
```

---

## 7. 创建测试框架 Mod

创建 `test.xfmod`：

```xfawa
#test {
    fn main() {
        int test_passed = 0
        int test_failed = 0
        
        fn test:describe(name) {
            print("=== Test: " + name + " ===")
        }
        
        fn test:expect(actual, expected, description) {
            if actual == expected {
                test_passed = test_passed + 1
                print("  PASS: " + description)
            }
            else {
                test_failed = test_failed + 1
                print("  FAIL: " + description)
                print("    Expected: " + expected)
                print("    Actual: " + actual)
            }
        }
        
        fn test:summary() {
            print("")
            print("=== Test Summary ===")
            print("Passed: " + test_passed)
            print("Failed: " + test_failed)
            if test_failed == 0 {
                print("All tests passed!")
            }
        }
    }
    
    @add {
        name: test
        syntax: test <name> { <action> }
        logic: {
            print("=== Test: " + <name> + " ===")
            <action>
        }
    }
    
    @add {
        name: expect
        syntax: expect <actual> equals <expected>
        logic: {
            if <actual> == <expected> {
                print("  PASS")
            }
            else {
                print("  FAIL")
                print("    Expected: " + <expected>)
                print("    Actual: " + <actual>)
            }
        }
    }
}
```

使用：

```xfawa
%import "test"
%import "math"

#app {
    fn main() {
        test:describe("Math functions")
        
        test:expect(math:square(5), 25, "square(5) should be 25")
        test:expect(math:cube(3), 27, "cube(3) should be 27")
        test:expect(math:max(10, 20), 20, "max(10, 20) should be 20")
        
        test:summary()
    }
}
```

---

## 8. 创建状态机 Mod

创建 `state_machine.xfmod`：

```xfawa
#state_machine {
    fn main() {
        fn sm:create() {
            print("State machine created")
            return 0
        }
        
        fn sm:transition(from, to) {
            print("Transition: " + from + " -> " + to)
        }
        
        fn sm:current(state) {
            print("Current state: " + state)
        }
    }
    
    @add {
        name: state
        syntax: state <name> { <action> }
        logic: {
            print("[STATE] " + <name>)
            <action>
        }
    }
    
    @add {
        name: transition
        syntax: transition to <state>
        logic: {
            print("-> Transitioning to: " + <state>)
        }
    }
    
    @add {
        name: onenter
        syntax: onenter { <action> }
        logic: {
            print("  [ENTER]")
            <action>
        }
    }
    
    @add {
        name: onexit
        syntax: onexit { <action> }
        logic: {
            print("  [EXIT]")
            <action>
        }
    }
}
```

使用：

```xfawa
%import "state_machine"

#app {
    fn main() {
        state "idle" {
            onenter {
                print("Waiting for input")
            }
            
            transition to "processing"
        }
        
        state "processing" {
            onenter {
                print("Processing data...")
            }
            
            transition to "complete"
        }
        
        state "complete" {
            onenter {
                print("Task finished!")
            }
        }
    }
}
```

---

## 9. 创建事件系统 Mod

创建 `events.xfmod`：

```xfawa
#events {
    fn main() {
        fn event:emit(name) {
            print("[EVENT] " + name + " emitted")
        }
        
        fn event:on(name) {
            print("[LISTEN] Listening for: " + name)
        }
    }
    
    @add {
        name: emit
        syntax: emit <event_name>
        logic: {
            print("[EVENT] " + <event_name> + " emitted")
        }
    }
    
    @add {
        name: on_event
        syntax: on <event_name> { <action> }
        logic: {
            print("[LISTEN] " + <event_name>)
            <action>
        }
    }
    
    @add {
        name: trigger
        syntax: trigger <event_name> with <data>
        logic: {
            print("[TRIGGER] " + <event_name> + " with data: " + <data>)
        }
    }
}
```

使用：

```xfawa
%import "events"

#app {
    fn main() {
        on "user_login" {
            print("User logged in!")
            emit "update_ui"
        }
        
        on "update_ui" {
            print("Refreshing UI...")
        }
        
        emit "user_login"
        
        trigger "data_received" with "user_data"
    }
}
```

---

## 10. 创建配置系统 Mod

创建 `config.xfmod`：

```xfawa
#config {
    fn main() {
        fn config:set(key, value) {
            print("[CONFIG] " + key + " = " + value)
        }
        
        fn config:get(key) {
            print("[CONFIG] Getting: " + key)
            return 0
        }
        
        fn config:load(filename) {
            print("[CONFIG] Loading from: " + filename)
        }
        
        fn config:save(filename) {
            print("[CONFIG] Saving to: " + filename)
        }
    }
    
    @add {
        name: setting
        syntax: setting <key> = <value>
        logic: {
            print("[CONFIG] " + <key> + " = " + <value>)
        }
    }
    
    @add {
        name: config_block
        syntax: config { <settings> }
        logic: {
            print("=== Configuration ===")
            <settings>
            print("=====================")
        }
    }
}
```

使用：

```xfawa
%import "config"

#app {
    fn main() {
        config {
            setting "difficulty" = "hard"
            setting "sound" = "on"
            setting "music" = "off"
        }
        
        config:load("game.cfg")
        config:set("volume", 80)
        config:save("game.cfg")
    }
}
```

---

## 11. Mod 组合使用

创建 `full_game.xfmod`：

```xfawa
#full_game {
    fn main() {
        fn game:init() {
            print("Initializing game...")
        }
        
        fn game:cleanup() {
            print("Cleaning up...")
        }
    }
    
    @add {
        name: game_loop
        syntax: game_loop <rounds> { <action> }
        logic: {
            int round = 1
            while (round <= <rounds>) {
                print("=== Round " + round + " ===")
                <action>
                round = round + 1
            }
        }
    }
    
    @add {
        name: game_over
        syntax: game_over { <action> }
        logic: {
            print("=== GAME OVER ===")
            <action>
        }
    }
}
```

使用多个 mod 组合：

```xfawa
%import "full_game"
%import "math"
%import "debug"

#app {
    fn main() {
        game:init()
        
        int score = 0
        
        game_loop 5 {
            int dice = rnd(1, 6)
            debug:trace("dice", dice)
            
            if dice >= 4 {
                score = math:add(score, dice * 10)
                print("Points: +" + (dice * 10))
            }
            else {
                print("No points")
            }
        }
        
        print("Final Score: " + score)
        
        game_over {
            if score >= 100 {
                print("Victory!")
            }
            else {
                print("Try again!")
            }
        }
        
        game:cleanup()
    }
}
```

---

## 最佳实践

### 1. Mod 命名规范

- 使用小写字母和下划线：`my_mod.xfmod`
- 名称应描述功能：`math.xfmod`, `debug.xfmod`
- 避免使用保留字

### 2. 命名空间规范

- 使用有意义的命名空间：`math:`, `game:`, `debug:`
- 保持命名空间的一致性
- 避免命名冲突

### 3. 文件组织

```
project/
├── mods/
│   ├── math.xfmod
│   ├── debug.xfmod
│   ├── game.xfmod
│   └── utils.xfmod
├── examples/
│   └── myapp.xf
└── main.xf
```

### 4. 导入顺序

```xfawa
// 推荐的导入顺序
%import "core_utils"      // 核心工具
%import "math"            // 数学库
%import "debug"           // 调试工具
%import "game"            // 游戏功能
```

### 5. 错误处理

在 mod 中提供清晰的错误信息：

```xfawa
#safe_math {
    fn main() {
        fn math:divide(a, b) {
            if b == 0 {
                print("[ERROR] Division by zero")
                return 0
            }
            return a / b
        }
    }
}
```

---

## 常见问题

### Q: 为什么只能使用 `%import`？

A: `%import` 是预处理指令，在编译的预处理阶段执行，确保 mod 在语法分析之前被加载。这样可以确保添加的新语法能够被正确解析。

### Q: 导入语句能被语法修改吗？

A: 不能。`%import` 是预处理指令，在语法修改之前就已经执行，因此无法被修改。

### Q: 如何处理多个 mod 的命名冲突？

A: 使用不同的命名空间来区分不同 mod 的函数。例如：`math:add()` 和 `utils:add()` 可以共存。

### Q: Mod 文件必须放在哪里？

A: 通常放在项目的 `mods/` 目录下，编译器会自动查找。也可以使用完整路径导入。

---

## 相关文档

- [Mods 系统基础](../ModsSystem.md)
- [基础语法](../BasicSyntax.md)
- [内置函数](../Builtins.md)
- [窗口系统](../WindowSystem.md)
