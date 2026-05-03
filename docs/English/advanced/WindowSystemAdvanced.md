# Window System Advanced

This document introduces advanced usage and combined techniques of the window system.

---

## Complex Interface Layout

### Calculator Application

```xfawa
#calculator {
    fn main() {
        window {
            width: 400
            height: 500
            title: "Calculator"
            color: lightblue
            
            text {
                x: 150
                y: 20
                width: 100
                height: 30
                text: "Calculator"
            }
            
            box {
                id: display
                x: 50
                y: 60
                width: 300
                height: 50
                text: "0"
            }
            
            button {
                x: 50
                y: 130
                width: 60
                height: 60
                text: "1"
                print("1", display)
            }
            
            button {
                x: 120
                y: 130
                width: 60
                height: 60
                text: "2"
                print("2", display)
            }
            
            button {
                x: 190
                y: 130
                width: 60
                height: 60
                text: "3"
                print("3", display)
            }
            
            button {
                x: 260
                y: 130
                width: 90
                height: 60
                text: "+"
                print("+", display)
            }
            
            button {
                x: 50
                y: 200
                width: 60
                height: 60
                text: "4"
                print("4", display)
            }
            
            button {
                x: 120
                y: 200
                width: 60
                height: 60
                text: "5"
                print("5", display)
            }
            
            button {
                x: 190
                y: 200
                width: 60
                height: 60
                text: "6"
                print("6", display)
            }
            
            button {
                x: 260
                y: 200
                width: 90
                height: 60
                text: "-"
                print("-", display)
            }
            
            button {
                x: 50
                y: 270
                width: 60
                height: 60
                text: "7"
                print("7", display)
            }
            
            button {
                x: 120
                y: 270
                width: 60
                height: 60
                text: "8"
                print("8", display)
            }
            
            button {
                x: 190
                y: 270
                width: 60
                height: 60
                text: "9"
                print("9", display)
            }
            
            button {
                x: 260
                y: 270
                width: 90
                height: 60
                text: "Clear"
                print("0", display)
            }
        }
    }
}
```

### Click Counter

```xfawa
#clicker {
    fn main() {
        window {
            width: 400
            height: 300
            title: "Click Counter"
            color: lightgreen
            
            text {
                x: 120
                y: 20
                width: 160
                height: 30
                text: "Click Counter"
            }
            
            box {
                id: score
                x: 150
                y: 70
                width: 100
                height: 50
                text: "0"
            }
            
            button {
                x: 100
                y: 150
                width: 200
                height: 60
                text: "CLICK ME!"
                
                int points = rnd(1, 10)
                print(points, score)
            }
            
            button {
                x: 150
                y: 230
                width: 100
                height: 40
                text: "Reset"
                
                print("0", score)
            }
        }
    }
}
```

---

## Interactive Components

### Simple Form

```xfawa
#simple_form {
    fn main() {
        window {
            width: 400
            height: 350
            title: "Simple Form"
            
            text {
                x: 150
                y: 20
                width: 100
                height: 30
                text: "User Form"
            }
            
            box {
                id: name_display
                x: 100
                y: 70
                width: 200
                height: 40
                text: "Name"
            }
            
            box {
                id: age_display
                x: 100
                y: 130
                width: 200
                height: 40
                text: "Age"
            }
            
            button {
                x: 50
                y: 200
                width: 100
                height: 40
                text: "Set Name"
                
                print("John", name_display)
            }
            
            button {
                x: 250
                y: 200
                width: 100
                height: 40
                text: "Set Age"
                
                print("25", age_display)
            }
            
            button {
                x: 100
                y: 270
                width: 200
                height: 40
                text: "Random Age"
                
                int age = rnd(18, 65)
                print(age, age_display)
            }
        }
    }
}
```

### Multi-page Layout

```xfawa
#multi_page {
    fn main() {
        window {
            width: 500
            height: 400
            title: "Multi Page"
            
            box {
                id: page1
                x: 50
                y: 50
                width: 400
                height: 250
                text: "Page 1 Content"
            }
            
            box {
                id: page2
                x: 50
                y: 50
                width: 400
                height: 250
                text: "Page 2 Content"
            }
            
            button {
                x: 100
                y: 320
                width: 100
                height: 40
                text: "Page 1"
                
                print("Page 1 Active", page1)
                print("Page 2 Inactive", page2)
            }
            
            button {
                x: 300
                y: 320
                width: 100
                height: 40
                text: "Page 2"
                
                print("Page 1 Inactive", page1)
                print("Page 2 Active", page2)
            }
        }
    }
}
```

---

## Game Interface Examples

### Number Guessing Game

```xfawa
#guess_game {
    fn main() {
        window {
            width: 400
            height: 350
            title: "Guess Game"
            
            text {
                x: 100
                y: 20
                width: 200
                height: 30
                text: "Guess 1-100"
            }
            
            box {
                id: result
                x: 100
                y: 70
                width: 200
                height: 50
                text: "Click to guess"
            }
            
            button {
                x: 50
                y: 150
                width: 100
                height: 50
                text: "Low (1-33)"
                
                int num = rnd(1, 33)
                print(num, result)
            }
            
            button {
                x: 150
                y: 150
                width: 100
                height: 50
                text: "Mid (34-66)"
                
                int num = rnd(34, 66)
                print(num, result)
            }
            
            button {
                x: 250
                y: 150
                width: 100
                height: 50
                text: "High (67-100)"
                
                int num = rnd(67, 100)
                print(num, result)
            }
            
            button {
                x: 100
                y: 230
                width: 200
                height: 50
                text: "Random Pick"
                
                int num = rnd(1, 100)
                print(num, result)
            }
        }
    }
}
```

### Simple Quiz

```xfawa
#simple_quiz {
    fn main() {
        window {
            width: 400
            height: 400
            title: "Quiz"
            
            text {
                x: 100
                y: 20
                width: 200
                height: 30
                text: "What is 5 + 3?"
            }
            
            box {
                id: feedback
                x: 100
                y: 280
                width: 200
                height: 50
                text: "Answer"
            }
            
            button {
                x: 100
                y: 80
                width: 200
                height: 50
                text: "5"
                
                print("Wrong! Try again", feedback)
            }
            
            button {
                x: 100
                y: 140
                width: 200
                height: 50
                text: "8"
                
                print("Correct!", feedback)
            }
            
            button {
                x: 100
                y: 200
                width: 200
                height: 50
                text: "10"
                
                print("Wrong! Try again", feedback)
            }
        }
    }
}
```

---

## Related Documentation

- [Window System](../WindowSystem.md)
- [Builtins Advanced](./BuiltinsAdvanced.md)
- [Control Flow Advanced](./ControlFlowAdvanced.md)
