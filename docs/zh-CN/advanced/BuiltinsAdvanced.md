# 内置函数高阶用法

本文档介绍内置函数的高级用法和组合技巧。

---

## print 高级用法

### 格式化输出

```xfawa
#formatted_output {
    fn main() {
        print("语言: xfawaPL")
        print("版本: 1.0")
        
        int score = 95
        print("得分:")
        print(score)
        
        if score >= 90 {
            print("评级: A")
        }
        else if score >= 80 {
            print("评级: B")
        }
        else if score >= 60 {
            print("评级: C")
        }
        else {
            print("评级: D")
        }
    }
}
```

### 多组件输出

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

## rnd 高级用法

### 概率系统

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

### 随机事件系统

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

## 随机数组选择

### 从数组中随机选择

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

### 随机颜色生成

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

## 组合示例

### 随机数游戏

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

### 骰子模拟器

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

## 相关文档

- [内置函数](../Builtins.md)
- [窗口系统高阶用法](./WindowSystemAdvanced.md)
- [控制流高阶用法](./ControlFlowAdvanced.md)
