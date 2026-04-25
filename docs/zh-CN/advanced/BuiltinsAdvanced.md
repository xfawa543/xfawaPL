# 内置函数高阶用法

本文档介绍内置函数的高级用法和组合技巧。

---

## print 高级用法

### 格式化输出

```xfawa
#formatted_output {
    fn main() {
        string name = "xfawaPL"
        int version = 1
        int subversion = 0
        
        print("语言: " + name)
        print("版本: " + version + "." + subversion)
        
        int score = 95
        print("得分: " + score + "分")
        print("评级: " + getGrade(score))
    }
    
    fn getGrade(score) {
        if score >= 90 {
            return "A"
        }
        else if score >= 80 {
            return "B"
        }
        else if score >= 60 {
            return "C"
        }
        else {
            return "D"
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
                print("Random: " + num, log1)
            }
            
            button {
                x: 250
                y: 220
                width: 150
                height: 40
                text: "Output to Log2"
                
                int num = rnd(1, 100)
                print("Random: " + num, log2)
            }
            
            button {
                x: 150
                y: 280
                width: 200
                height: 40
                text: "Output to Both"
                
                int num = rnd(1, 100)
                print("Same: " + num, log1)
                print("Same: " + num, log2)
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
                height: 50
                text: "Trigger Event"
                
                int events = [1, 2, 3, 4, 5]
                int eventType = rnd(events)
                
                if eventType == 1 {
                    print("金币掉落!", event)
                }
                else if eventType == 2 {
                    print("怪物出现!", event)
                }
                else if eventType == 3 {
                    print("发现宝箱!", event)
                }
                else if eventType == 4 {
                    print("获得经验!", event)
                }
                else {
                    print("什么都没发生", event)
                }
            }
        }
    }
}
```

---

## 组合应用

### 骰子游戏

```xfawa
#dice_game {
    fn main() {
        int playerScore = 0
        int computerScore = 0
        
        window {
            width: 400
            height: 400
            title: "Dice Game"
            color: lightblue
            
            text {
                x: 150
                y: 10
                width: 100
                height: 30
                text: "Dice Game"
            }
            
            box {
                id: playerDice
                x: 50
                y: 50
                width: 100
                height: 50
                text: "Player: 0"
            }
            
            box {
                id: computerDice
                x: 250
                y: 50
                width: 100
                height: 50
                text: "Computer: 0"
            }
            
            box {
                id: result
                x: 100
                y: 120
                width: 200
                height: 40
                text: "Roll to start!"
            }
            
            box {
                id: score
                x: 100
                y: 180
                width: 200
                height: 40
                text: "Score: 0 - 0"
            }
            
            button {
                x: 100
                y: 250
                width: 200
                height: 50
                text: "Roll Dice"
                
                int playerRoll = rnd(1, 6)
                int computerRoll = rnd(1, 6)
                
                print("Player: " + playerRoll, playerDice)
                print("Computer: " + computerRoll, computerDice)
                
                if playerRoll > computerRoll {
                    print("You win!", result)
                }
                else if playerRoll < computerRoll {
                    print("Computer wins!", result)
                }
                else {
                    print("Tie!", result)
                }
            }
        }
    }
}
```

### 抽奖系统

```xfawa
#lottery {
    fn main() {
        window {
            width: 400
            height: 400
            title: "Lottery"
            color: yellow
            
            text {
                x: 150
                y: 20
                width: 100
                height: 30
                text: "Lottery"
            }
            
            box {
                id: prize
                x: 100
                y: 70
                width: 200
                height: 60
                text: "Click to draw!"
            }
            
            box {
                id: history
                x: 50
                y: 150
                width: 300
                height: 150
                text: "History:"
            }
            
            button {
                x: 100
                y: 320
                width: 200
                height: 40
                text: "Draw Prize"
                
                int prizes = [1, 2, 3, 4, 5, 6]
                int result = rnd(prizes)
                
                if result == 1 {
                    print("一等奖!", prize)
                }
                else if result == 2 {
                    print("二等奖!", prize)
                }
                else if result == 3 {
                    print("三等奖!", prize)
                }
                else {
                    print("谢谢参与!", prize)
                }
                
                print(result, history)
            }
        }
    }
}
```

### 猜数字游戏

```xfawa
#guess_number {
    fn main() {
        int target = rnd(1, 100)
        int attempts = 0
        
        window {
            width: 400
            height: 350
            title: "Guess Number"
            
            text {
                x: 100
                y: 20
                width: 200
                height: 30
                text: "Guess 1-100"
            }
            
            box {
                id: message
                x: 100
                y: 60
                width: 200
                height: 40
                text: "Make a guess!"
            }
            
            box {
                id: attempts_box
                x: 100
                y: 110
                width: 200
                height: 30
                text: "Attempts: 0"
            }
            
            button {
                x: 50
                y: 160
                width: 100
                height: 40
                text: "Guess 1-25"
                
                int guess = rnd(1, 25)
                attempts = attempts + 1
                print("Attempts: " + attempts, attempts_box)
                
                if guess == target {
                    print("Correct! Answer: " + target, message)
                }
                else if guess < target {
                    print("Too low! Guess: " + guess, message)
                }
                else {
                    print("Too high! Guess: " + guess, message)
                }
            }
            
            button {
                x: 150
                y: 160
                width: 100
                height: 40
                text: "Guess 26-50"
                
                int guess = rnd(26, 50)
                attempts = attempts + 1
                print("Attempts: " + attempts, attempts_box)
                
                if guess == target {
                    print("Correct! Answer: " + target, message)
                }
                else if guess < target {
                    print("Too low! Guess: " + guess, message)
                }
                else {
                    print("Too high! Guess: " + guess, message)
                }
            }
            
            button {
                x: 250
                y: 160
                width: 100
                height: 40
                text: "Guess 51-75"
                
                int guess = rnd(51, 75)
                attempts = attempts + 1
                print("Attempts: " + attempts, attempts_box)
                
                if guess == target {
                    print("Correct! Answer: " + target, message)
                }
                else if guess < target {
                    print("Too low! Guess: " + guess, message)
                }
                else {
                    print("Too high! Guess: " + guess, message)
                }
            }
            
            button {
                x: 150
                y: 220
                width: 100
                height: 40
                text: "Guess 76-100"
                
                int guess = rnd(76, 100)
                attempts = attempts + 1
                print("Attempts: " + attempts, attempts_box)
                
                if guess == target {
                    print("Correct! Answer: " + target, message)
                }
                else if guess < target {
                    print("Too low! Guess: " + guess, message)
                }
                else {
                    print("Too high! Guess: " + guess, message)
                }
            }
            
            button {
                x: 150
                y: 280
                width: 100
                height: 40
                text: "New Game"
                
                target = rnd(1, 100)
                attempts = 0
                print("New game started!", message)
                print("Attempts: 0", attempts_box)
            }
        }
    }
}
```

---

## 相关文档

- [内置函数基础](../Builtins.md)
- [窗口系统高阶用法](./WindowSystemAdvanced.md)
- [控制流高阶用法](./ControlFlowAdvanced.md)
