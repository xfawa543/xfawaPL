# Mods Advanced

This document introduces advanced usage and combined techniques of the Mods system.

---

## 1. Multiple Mod Combination

```xfawa
%import "prob"
%import "repeat"
%import "utils"

#app {
    fn main() {
        prob 30 50 {
            print("Triggered!")
        }
        
        repeat 5 {
            print("Repeat!")
        }
        
        pub:greet("World")
    }
}
```

---

## 2. Syntax Modification + Syntax Addition Combination

Create `mylang.xfmod`:

```xfawa
#mylang {
    fn main() {
        "print" => "say"
        "if" => "when"
        "else" => "otherwise"
    }
    
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
        say("Hello!")
        
        when true {
            say("True!")
        }
        otherwise {
            say("False!")
        }
        
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
            print(x * x)
        }
        
        fn math:cube(x) {
            print(x * x * x)
        }
        
        fn math:power(base, exp) {
            int result = 1
            int i = 0
            while (i < exp) {
                result = result * base
                i = i + 1
            }
            print(result)
        }
        
        fn math:max(a, b) {
            if a > b {
                print(a)
            }
            else {
                print(b)
            }
        }
        
        fn math:min(a, b) {
            if a < b {
                print(a)
            }
            else {
                print(b)
            }
        }
        
        fn math:abs(x) {
            if x < 0 {
                print(0 - x)
            }
            else {
                print(x)
            }
        }
        
        fn math:factorial(n) {
            if n <= 1 {
                print(1)
            }
            else {
                int result = 1
                int i = 2
                while (i <= n) {
                    result = result * i
                    i = i + 1
                }
                print(result)
            }
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
        math:square(5)
        
        print("Cube of 3:")
        math:cube(3)
        
        print("2 to the power of 10:")
        math:power(2, 10)
        
        print("Max of 10 and 20:")
        math:max(10, 20)
        
        print("Factorial of 5:")
        math:factorial(5)
    }
}
```

---

## 4. Creating Game Utility Library

Create `game.xfmod`:

```xfawa
#game {
    fn main() {
        fn game:rollDice() {
            print(rnd(1, 6))
        }
        
        fn game:rollMultiple(count) {
            int i = 0
            while (i < count) {
                print(rnd(1, 6))
                i = i + 1
            }
        }
        
        fn game:coinFlip() {
            int result = rnd(0, 1)
            if result == 0 {
                print("Heads")
            }
            else {
                print("Tails")
            }
        }
        
        fn game:randomRange(min, max) {
            print(rnd(min, max))
        }
    }
}
```

Usage:

```xfawa
%import "game"

#app {
    fn main() {
        print("Roll a dice:")
        game:rollDice()
        
        print("Roll 3 dice:")
        game:rollMultiple(3)
        
        print("Flip a coin:")
        game:coinFlip()
        
        print("Random 1-100:")
        game:randomRange(1, 100)
    }
}
```

---

## 5. Creating String Utility Library

Create `str.xfmod`:

```xfawa
#str {
    fn main() {
        fn str:repeat(text, count) {
            int i = 0
            while (i < count) {
                print(text)
                i = i + 1
            }
        }
        
        fn str:line(length) {
            int i = 0
            while (i < length) {
                print("-")
                i = i + 1
            }
        }
        
        fn str:box(text) {
            print("+" + "----" + "+")
            print("| " + text + " |")
            print("+" + "----" + "+")
        }
    }
}
```

Usage:

```xfawa
%import "str"

#app {
    fn main() {
        print("Repeat Hello 3 times:")
        str:repeat("Hello", 3)
        
        print("Line of 10 dashes:")
        str:line(10)
        
        print("Box around text:")
        str:box("Test")
    }
}
```

---

## Related Documentation

- [Mods System](../ModsSystem.md)
- [Builtins Advanced](./BuiltinsAdvanced.md)
- [Basic Syntax Advanced](./BasicSyntaxAdvanced.md)
