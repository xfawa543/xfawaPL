# Mods Advanced Usage

This document introduces advanced usage and combination techniques for the Mods system.

---

## 1. Multiple Mods Combination

```xfawa
%import "prob"
%import "repeat"
%import "utils"

#app {
    fn main() {
        // Use prob mod syntax
        prob 30 50 {
            print("Triggered!")
        }
        
        // Use repeat mod syntax
        repeat 5 {
            print("Repeat!")
        }
        
        // Use utils mod functions
        pub:greet("World")
    }
}
```

---

## 2. Syntax Modification + Addition Combination

Create `mylang.xfmod`:

```xfawa
#mylang {
    fn main() {
        // Modify keywords
        "print" => "say"
        "if" => "when"
        "else" => "otherwise"
    }
    
    // Add new syntax
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

Usage:

```xfawa
%import "mylang"

#app {
    fn main() {
        // Use modified keywords
        say("Hello!")
        
        when true {
            say("True!")
        }
        otherwise {
            say("False!")
        }
        
        // Use added new syntax
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

## 3. Creating Math Function Library

Create `math.xfmod`:

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

Usage:

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

## 4. Creating Game Tools Library

Create `game.xfmod`:

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

Usage:

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

## 5. Complex Custom Syntax

Create `advanced.xfmod`:

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

Usage:

```xfawa
%import "advanced"

#app {
    fn main() {
        // loop syntax
        loop i from 1 to 5 {
            print(i)
        }
        
        // countdown syntax
        countdown 3 {
            print("Go!")
        }
        
        // unless syntax
        int x = 5
        unless x == 10 {
            print("x is not 10")
        }
        
        // when syntax
        when x == 5 do print("x is 5")
    }
}
```

---

## 6. Creating Debug Tools Mod

Create `debug.xfmod`:

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

Usage:

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

## 7. Creating Test Framework Mod

Create `test.xfmod`:

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

Usage:

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

## 8. Creating State Machine Mod

Create `state_machine.xfmod`:

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

Usage:

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

## 9. Creating Event System Mod

Create `events.xfmod`:

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

Usage:

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

## 10. Creating Configuration System Mod

Create `config.xfmod`:

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

Usage:

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

## 11. Mod Combination Usage

Create `full_game.xfmod`:

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

Using multiple mods together:

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

## Best Practices

### 1. Mod Naming Conventions

- Use lowercase letters and underscores: `my_mod.xfmod`
- Names should describe functionality: `math.xfmod`, `debug.xfmod`
- Avoid using reserved words

### 2. Namespace Conventions

- Use meaningful namespaces: `math:`, `game:`, `debug:`
- Maintain namespace consistency
- Avoid naming conflicts

### 3. File Organization

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

### 4. Import Order

```xfawa
// Recommended import order
%import "core_utils"      // Core utilities
%import "math"            // Math library
%import "debug"           // Debug tools
%import "game"            // Game functions
```

### 5. Error Handling

Provide clear error messages in mods:

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

## FAQ

### Q: Why can only `%import` be used?

A: `%import` is a preprocessor directive that executes during compilation preprocessing, ensuring mods are loaded before syntax analysis. This ensures that newly added syntax can be correctly parsed.

### Q: Can import statements be modified by syntax modification?

A: No. `%import` is a preprocessor directive that executes before syntax modification, so it cannot be modified.

### Q: How to handle naming conflicts between multiple mods?

A: Use different namespaces to distinguish functions from different mods. For example: `math:add()` and `utils:add()` can coexist.

### Q: Where must mod files be placed?

A: Typically in the project's `mods/` directory, where the compiler automatically searches. You can also import using full paths.

---

## Related Documents

- [Mods System Basics](../ModsSystem.md)
- [Basic Syntax](../BasicSyntax.md)
- [Built-in Functions](../Builtins.md)
- [Window System](../WindowSystem.md)
