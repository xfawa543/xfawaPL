# 窗口系统高阶用法

本文档介绍窗口系统的高级用法和组合技巧。

---

## 复杂界面布局

### 计算器应用

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

### 点击计数器

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

## 多组件交互

### 日志查看器

```xfawa
#log_viewer {
    fn main() {
        window {
            width: 500
            height: 400
            title: "Log Viewer"
            
            text {
                x: 200
                y: 10
                width: 100
                height: 30
                text: "Log Viewer"
            }
            
            box {
                id: logbox
                x: 20
                y: 50
                width: 460
                height: 280
                text: "Logs will appear here..."
            }
            
            button {
                x: 50
                y: 350
                width: 120
                height: 35
                text: "Add Log"
                
                int timestamp = rnd(1000, 9999)
                print("[" + timestamp + "] Log entry", logbox)
            }
            
            button {
                x: 200
                y: 350
                width: 100
                height: 35
                text: "Clear"
                
                print("Logs cleared.", logbox)
            }
            
            button {
                x: 330
                y: 350
                width: 120
                height: 35
                text: "Random"
                
                int values = [10, 20, 30, 40, 50]
                int val = rnd(values)
                print("Random: " + val, logbox)
            }
        }
    }
}
```

### 多面板界面

```xfawa
#multi_panel {
    fn main() {
        window {
            width: 600
            height: 400
            title: "Multi Panel"
            
            text {
                x: 250
                y: 10
                width: 100
                height: 30
                text: "Dashboard"
            }
            
            box {
                id: panel1
                x: 20
                y: 50
                width: 180
                height: 200
                text: "Panel 1"
            }
            
            box {
                id: panel2
                x: 210
                y: 50
                width: 180
                height: 200
                text: "Panel 2"
            }
            
            box {
                id: panel3
                x: 400
                y: 50
                width: 180
                height: 200
                text: "Panel 3"
            }
            
            button {
                x: 50
                y: 270
                width: 120
                height: 40
                text: "Update P1"
                
                int val = rnd(1, 100)
                print("P1: " + val, panel1)
            }
            
            button {
                x: 240
                y: 270
                width: 120
                height: 40
                text: "Update P2"
                
                int val = rnd(1, 100)
                print("P2: " + val, panel2)
            }
            
            button {
                x: 430
                y: 270
                width: 120
                height: 40
                text: "Update P3"
                
                int val = rnd(1, 100)
                print("P3: " + val, panel3)
            }
            
            button {
                x: 200
                y: 330
                width: 200
                height: 40
                text: "Update All"
                
                int v1 = rnd(1, 100)
                int v2 = rnd(1, 100)
                int v3 = rnd(1, 100)
                print("All: " + v1, panel1)
                print("All: " + v2, panel2)
                print("All: " + v3, panel3)
            }
        }
    }
}
```

---

## 游戏界面

### 简易RPG界面

```xfawa
#rpg_interface {
    fn main() {
        int hp = 100
        int mp = 50
        int gold = 0
        
        window {
            width: 500
            height: 450
            title: "Simple RPG"
            color: lightblue
            
            text {
                x: 200
                y: 10
                width: 100
                height: 30
                text: "Simple RPG"
            }
            
            box {
                id: hp_display
                x: 50
                y: 50
                width: 100
                height: 40
                text: "HP: 100"
            }
            
            box {
                id: mp_display
                x: 160
                y: 50
                width: 100
                height: 40
                text: "MP: 50"
            }
            
            box {
                id: gold_display
                x: 270
                y: 50
                width: 100
                height: 40
                text: "Gold: 0"
            }
            
            box {
                id: message
                x: 50
                y: 100
                width: 400
                height: 80
                text: "Welcome to the game!"
            }
            
            box {
                id: log
                x: 50
                y: 190
                width: 400
                height: 120
                text: "Game Log:"
            }
            
            button {
                x: 50
                y: 330
                width: 120
                height: 40
                text: "Explore"
                
                int event = rnd(1, 4)
                if event == 1 {
                    int found = rnd(5, 20)
                    print("Found " + found + " gold!", message)
                    print("Gold +" + found, log)
                }
                else if event == 2 {
                    int damage = rnd(5, 15)
                    print("Encountered enemy! -" + damage + " HP", message)
                    print("Battle -" + damage + " HP", log)
                }
                else if event == 3 {
                    int heal = rnd(10, 25)
                    print("Found healing potion! +" + heal + " HP", message)
                    print("Healed +" + heal + " HP", log)
                }
                else {
                    print("Nothing happened...", message)
                    print("Empty area", log)
                }
            }
            
            button {
                x: 190
                y: 330
                width: 120
                height: 40
                text: "Rest"
                
                print("You rest and recover.", message)
                print("Rested", log)
            }
            
            button {
                x: 330
                y: 330
                width: 120
                height: 40
                text: "Shop"
                
                print("Welcome to the shop!", message)
                print("Visited shop", log)
            }
        }
    }
}
```

### 反应速度测试

```xfawa
#reaction_test {
    fn main() {
        int score = 0
        int round = 0
        
        window {
            width: 400
            height: 350
            title: "Reaction Test"
            
            text {
                x: 150
                y: 20
                width: 100
                height: 30
                text: "Reaction Test"
            }
            
            box {
                id: target
                x: 150
                y: 70
                width: 100
                height: 100
                text: "Wait..."
            }
            
            box {
                id: score_display
                x: 100
                y: 190
                width: 200
                height: 40
                text: "Score: 0"
            }
            
            button {
                x: 100
                y: 250
                width: 200
                height: 50
                text: "Click When GO!"
                
                int reaction = rnd(1, 3)
                if reaction == 1 {
                    print("GO!", target)
                    score = score + 1
                    print("Score: " + score, score_display)
                }
                else {
                    print("Too early!", target)
                }
            }
            
            button {
                x: 150
                y: 310
                width: 100
                height: 30
                text: "Reset"
                
                score = 0
                print("Wait...", target)
                print("Score: 0", score_display)
            }
        }
    }
}
```

---

## 数据可视化

### 简易图表

```xfawa
#simple_chart {
    fn main() {
        window {
            width: 500
            height: 400
            title: "Simple Chart"
            
            text {
                x: 200
                y: 10
                width: 100
                height: 30
                text: "Data Chart"
            }
            
            box {
                id: data1
                x: 50
                y: 50
                width: 80
                height: 200
                text: ""
            }
            
            box {
                id: data2
                x: 140
                y: 50
                width: 80
                height: 200
                text: ""
            }
            
            box {
                id: data3
                x: 230
                y: 50
                width: 80
                height: 200
                text: ""
            }
            
            box {
                id: data4
                x: 320
                y: 50
                width: 80
                height: 200
                text: ""
            }
            
            box {
                id: values
                x: 50
                y: 260
                width: 350
                height: 60
                text: "Values: -"
            }
            
            button {
                x: 150
                y: 340
                width: 200
                height: 40
                text: "Generate Data"
                
                int v1 = rnd(10, 100)
                int v2 = rnd(10, 100)
                int v3 = rnd(10, 100)
                int v4 = rnd(10, 100)
                
                print(v1, data1)
                print(v2, data2)
                print(v3, data3)
                print(v4, data4)
                print("Values: " + v1 + ", " + v2 + ", " + v3 + ", " + v4, values)
            }
        }
    }
}
```

---

## 相关文档

- [窗口系统基础](../WindowSystem.md)
- [内置函数高阶用法](./BuiltinsAdvanced.md)
- [控制流高阶用法](./ControlFlowAdvanced.md)
