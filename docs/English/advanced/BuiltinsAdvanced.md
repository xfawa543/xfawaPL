# Builtins Advanced

This document introduces advanced usage and combined techniques of built-in functions.

---

## Advanced print Usage

### Formatted Output

```xfawa
#formatted_output {
    fn main() {
        print("Language: xfawaPL")
        print("Version: 1.0")
        
        int score = 95
        print("Score:")
        print(score)
        
        if score >= 90 {
            print("Grade: A")
        }
        else if score >= 80 {
            print("Grade: B")
        }
        else if score >= 60 {
            print("Grade: C")
        }
        else {
            print("Grade: D")
        }
    }
}
```

### Multi-component Output

```xfawa
#multi_component {
    fn main() {
        window {
            width: 500
            height: 400
            title: "Multi Output"
            
            box {
                id: log1
                x: 20
                y: 50
                width: 220
                height: 150
                text: "Log 1"
            }
            
            box {
                id: log2
                x: 260
                y: 50
                width: 220
                height: 150
                text: "Log 2"
            }
            
            button {
                x: 50
                y: 220
                width: 150
                height: 40
                text: "Output to Log1"
                
                int num = rnd(1, 100)
                print("Random:", log1)
                print(num, log1)
            }
            
            button {
                x: 250
                y: 220
                width: 150
                height: 40
                text: "Output to Log2"
                
                int num = rnd(1, 100)
                print("Random:", log2)
                print(num, log2)
            }
            
            button {
                x: 150
                y: 280
                width: 200
                height: 40
                text: "Output to Both"
                
                int num = rnd(1, 100)
                print("Same:", log1)
                print(num, log1)
                print("Same:", log2)
                print(num, log2)
            }
        }
    }
}
```

---

## Advanced rnd Usage

### Probability System

```xfawa
#probability_system {
    fn main() {
        window {
            width: 400
            height: 350
            title: "Probability System"
            
            box {
                id: result
                x: 100
                y: 50
                width: 200
                height: 50
                text: "Result"
            }
            
            button {
                x: 100
                y: 120
                width: 200
                height: 40
                text: "50% Chance"
                
                int chance = rnd(1, 100)
                if chance <= 50 {
                    print("Success!", result)
                }
                else {
                    print("Failed!", result)
                }
            }
            
            button {
                x: 100
                y: 180
                width: 200
                height: 40
                text: "30% Chance"
                
                int chance = rnd(1, 100)
                if chance <= 30 {
                    print("Success!", result)
                }
                else {
                    print("Failed!", result)
                }
            }
            
            button {
                x: 100
                y: 240
                width: 200
                height: 40
                text: "10% Chance"
                
                int chance = rnd(1, 100)
                if chance <= 10 {
                    print("Success!", result)
                }
                else {
                    print("Failed!", result)
                }
            }
        }
    }
}
```

### Random Event System

```xfawa
#random_events {
    fn main() {
        window {
            width: 400
            height: 300
            title: "Random Events"
            
            box {
                id: event
                x: 50
                y: 50
                width: 300
                height: 100
                text: "Click to trigger event"
            }
            
            button {
                x: 100
                y: 180
                width: 200
                height: 40
                text: "Random Event"
                
                int eventType = rnd(1, 4)
                
                if eventType == 1 {
                    print("Event: Treasure found!", event)
                }
                else if eventType == 2 {
                    print("Event: Enemy appeared!", event)
                }
                else if eventType == 3 {
                    print("Event: Nothing happened", event)
                }
                else {
                    print("Event: Lucky bonus!", event)
                }
            }
        }
    }
}
```

---

## Random Array Selection

### Select from Array

```xfawa
#random_array {
    fn main() {
        int options = [1, 2, 3, 4, 5]
        
        print("Random selection from array:")
        print(rnd(options))
        
        print("Random from range [1, 100]:")
        print(rnd(1, 100))
    }
}
```

### Random Color Generation

```xfawa
#random_color {
    fn main() {
        window {
            width: 400
            height: 300
            title: "Random Color"
            
            box {
                id: colorBox
                x: 100
                y: 50
                width: 200
                height: 100
                text: "Color Box"
            }
            
            button {
                x: 100
                y: 180
                width: 200
                height: 40
                text: "Random Color"
                
                int r = rnd(0, 255)
                int g = rnd(0, 255)
                int b = rnd(0, 255)
                
                print("R:", colorBox)
                print(r, colorBox)
                print("G:", colorBox)
                print(g, colorBox)
                print("B:", colorBox)
                print(b, colorBox)
            }
        }
    }
}
```

---

## Combined Examples

### Number Guessing Game

```xfawa
#number_game {
    fn main() {
        window {
            width: 400
            height: 350
            title: "Number Guessing Game"
            
            box {
                id: hint
                x: 50
                y: 50
                width: 300
                height: 50
                text: "Guess a number 1-100"
            }
            
            box {
                id: result
                x: 50
                y: 120
                width: 300
                height: 50
                text: "Result"
            }
            
            button {
                x: 50
                y: 200
                width: 100
                height: 40
                text: "Guess 1-33"
                
                int target = rnd(1, 100)
                int guess = rnd(1, 33)
                
                print("Target:", result)
                print(target, result)
                print("Your guess:", result)
                print(guess, result)
                
                if guess == target {
                    print("Correct!", hint)
                }
                else {
                    print("Wrong!", hint)
                }
            }
            
            button {
                x: 150
                y: 200
                width: 100
                height: 40
                text: "Guess 34-66"
                
                int target = rnd(1, 100)
                int guess = rnd(34, 66)
                
                print("Target:", result)
                print(target, result)
                print("Your guess:", result)
                print(guess, result)
                
                if guess == target {
                    print("Correct!", hint)
                }
                else {
                    print("Wrong!", hint)
                }
            }
            
            button {
                x: 250
                y: 200
                width: 100
                height: 40
                text: "Guess 67-100"
                
                int target = rnd(1, 100)
                int guess = rnd(67, 100)
                
                print("Target:", result)
                print(target, result)
                print("Your guess:", result)
                print(guess, result)
                
                if guess == target {
                    print("Correct!", hint)
                }
                else {
                    print("Wrong!", hint)
                }
            }
        }
    }
}
```

### Dice Simulator

```xfawa
#dice_simulator {
    fn main() {
        window {
            width: 400
            height: 300
            title: "Dice Simulator"
            
            box {
                id: dice
                x: 150
                y: 50
                width: 100
                height: 100
                text: "Roll!"
            }
            
            button {
                x: 100
                y: 180
                width: 200
                height: 40
                text: "Roll Dice"
                
                int result = rnd(1, 6)
                print(result, dice)
            }
        }
    }
}
```

---

## Related Documentation

- [Builtins](../Builtins.md)
- [Window System Advanced](./WindowSystemAdvanced.md)
- [Control Flow Advanced](./ControlFlowAdvanced.md)
