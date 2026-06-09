# 窗口系统 (Window System)

本文档介绍 xfawaPL 的窗口系统 (Xraphics)。

---

## window 语句

创建原生窗口应用程序。

### 语法

```xfawa
window {
    width: <宽度>
    height: <高度>
    title: "<标题>"
    color: <颜色>
}
```

### 窗口属性

| 属性 | 类型 | 说明 | 示例 |
|------|------|------|------|
| `width` | int | 窗口宽度 | `width: 400` |
| `height` / `high` | int | 窗口高度 | `height: 300` |
| `title` | string | 窗口标题 | `title: "My App"` |
| `color` | string | 窗口背景颜色 | `color: red` |
| `style` | string | 窗口样式 | `style: modern` |

### 支持的颜色

| 颜色名 | 说明 |
|--------|------|
| `red` | 红色 |
| `green` | 绿色 |
| `blue` | 蓝色 |
| `white` | 白色 |
| `black` | 黑色 |
| `yellow` | 黄色 |
| `cyan` | 青色 |
| `lightblue` | 浅蓝色 |
| `lightgreen` | 浅绿色 |

### 示例

```xfawa
#simple_window {
    fn main() {
        window {
            width: 420
            height: 260
            title: "Simple Window"
            color: white
        }
    }
}
```

---

## text 组件

在窗口中添加文本显示。

### 语法

```xfawa
text {
    x: <X坐标>
    y: <Y坐标>
    width: <宽度>
    height: <高度>
    text: "<文本内容>"
}
```

### 属性

| 属性 | 类型 | 说明 |
|------|------|------|
| `x` | int | X坐标 |
| `y` | int | Y坐标 |
| `width` | int | 文本区域宽度 |
| `height` | int | 文本区域高度 |
| `text` | string | 显示的文本 |

### 示例

```xfawa
#text_demo {
    fn main() {
        window {
            width: 400
            height: 200
            title: "Text Demo"
            
            text {
                x: 100
                y: 50
                width: 200
                height: 30
                text: "Hello, xfawaPL!"
            }
        }
    }
}
```

---

## button 组件

在窗口中添加按钮。

### 语法

```xfawa
button {
    x: <X坐标>
    y: <Y坐标>
    width: <宽度>
    height: <高度>
    text: "<按钮文本>"

    // 按钮点击时执行的代码
    <代码>
}
```

### 属性

| 属性 | 类型 | 说明 |
|------|------|------|
| `x` | int | X坐标 |
| `y` | int | Y坐标 |
| `width` | int | 按钮宽度 |
| `height` / `high` | int | 按钮高度 |
| `text` / `title` | string | 按钮文本 |

### 示例

```xfawa
#button_demo {
    fn main() {
        window {
            width: 400
            height: 200
            title: "Button Demo"

            button {
                x: 100
                y: 80
                width: 200
                height: 40
                text: "Click Me"

                print("Button clicked!")
            }
        }
    }
}
```

---

## XSS 样式系统

使用 XSS (xfawa Style Sheets) 文件定义组件样式。

### 语法

在 window 中指定样式文件：

```xfawa
window {
    width: 600
    height: 400
    title: "Styled Window"
    color: white
    style: "styles.xss"

    // 组件...
}
```

### XSS 文件格式

```css
button {
    background-color: #2d7ff9
    text-color: white
    border-radius: 8
    border-width: 1
    border-color: #1d4fa0
}
```

### 样式属性

| 属性 | 类型 | 说明 |
|------|------|------|
| `background-color` | color | 背景颜色（十六进制） |
| `text-color` | color | 文本颜色 |
| `border-radius` | int | 圆角半径 |
| `border-width` | int | 边框宽度 |
| `border-color` | color | 边框颜色 |

---

## 动画系统

在 XSS 文件中定义组件动画效果。

### 动画参数

| 参数 | 说明 | 示例 |
|------|------|------|
| `x-animation` | X轴移动动画 | `x-animation: animation(10, 200)` |
| `y-animation` | Y轴移动动画 | `y-animation: animation(50, 150)` |
| `width-animation` | 宽度变化动画 | `width-animation: animation(100, 300)` |
| `height-animation` | 高度变化动画 | `height-animation: animation(40, 80)` |

### 动画语法

```css
animation(初始值, 目标值)
```

- **初始值**：动画开始时的值（像素）
- **目标值**：动画结束时的值（像素）

### 动画触发器

使用 `animation-trigger {}` 子块定义动画触发条件。只有在触发器块内定义的动画参数才会受到触发器控制。

#### 触发器语法

```css
button {
    // 样式属性...

    animation-trigger {
        // 动画参数（在触发器块内）
        x-animation: animation(10, 200)
        y-animation: animation(50, 150)
        width-animation: animation(100, 300)
        height-animation: animation(40, 80)

        // 触发条件
        window-click()           // 全窗口点击触发
        window-click(x, y, w, h) // 指定区域点击触发
        key-click: "A"           // 按键触发
        button-click: "命名空间"  // 按钮命名空间触发
    }
}
```

#### 触发条件类型

| 触发器 | 语法 | 说明 |
|--------|------|------|
| **window-click** | `window-click()` | 点击窗口任意位置触发 |
| **window-click** | `window-click(x, y, w, h)` | 点击指定区域触发（x, y为区域左上角，w, h为区域宽高） |
| **key-click** | `key-click: "键名"` | 按键触发（支持A-Z字母键） |
| **button-click** | `button-click: "命名空间"` | 点击指定命名空间的按钮触发 |

### 动画示例

#### 示例1：窗口点击触发

```xfawa
#window_click_demo {
    fn main() {
        window {
            width: 600
            height: 400
            title: "Window Click Animation"
            color: white
            style: "animation.xss"

            text {
                x: 10
                y: 10
                text: "Click anywhere to trigger animation"
            }

            button {
                x: 10
                y: 100
                width: 120
                height: 40
                text: "Animated Button"
                print("Button clicked")
            }
        }
    }
}
```

**animation.xss:**

```css
button {
    background-color: #2d7ff9
    text-color: white
    border-radius: 8
    border-width: 1
    border-color: #1d4fa0

    animation-trigger {
        x-animation: animation(10, 400)
        y-animation: animation(100, 100)
        width-animation: animation(120, 200)
        height-animation: animation(40, 80)
        window-click()
    }
}
```

#### 示例2：按键触发

```xfawa
#key_click_demo {
    fn main() {
        window {
            width: 600
            height: 400
            title: "Key Click Animation"
            color: white
            style: "key_animation.xss"

            text {
                x: 10
                y: 10
                text: "Press 'A' key to trigger animation"
            }

            button {
                x: 10
                y: 100
                width: 120
                height: 40
                text: "Animated Button"
                print("Button clicked")
            }
        }
    }
}
```

**key_animation.xss:**

```css
button {
    background-color: #f92d7f
    text-color: white
    border-radius: 8
    border-width: 1
    border-color: #a01d4f

    animation-trigger {
        x-animation: animation(10, 400)
        y-animation: animation(100, 100)
        width-animation: animation(120, 200)
        height-animation: animation(40, 80)
        key-click: "A"
    }
}
```

### 动画行为说明

1. **自动播放控制**：动画参数在 `animation-trigger {}` 块内时，不会在程序启动时自动播放
2. **触发激活**：只有满足触发条件时，动画才会被激活并开始执行
3. **动画顺序**：多个动画参数同时执行，从初始值线性插值到目标值
4. **单位**：所有动画值的单位为像素

---

## box 组件

在窗口中添加可更新的文本框，支持滚动。

### 语法

```xfawa
box {
    id: <标识符>
    x: <X坐标>
    y: <Y坐标>
    width: <宽度>
    height: <高度>
    text: "<初始文本>"
}
```

### 属性

| 属性 | 类型 | 说明 |
|------|------|------|
| `id` | string | box 的唯一标识符 |
| `x` | int | X坐标 |
| `y` | int | Y坐标 |
| `width` | int | box 宽度 |
| `height` | int | box 高度 |
| `text` | string | 初始文本内容 |

### 更新 box 内容

使用 `print` 函数更新 box 内容：

```xfawa
print("新内容", box_id)
```

### 滚动功能

当 box 内容超出可见区域时，可以使用鼠标滚轮上下滚动查看。

### 示例

```xfawa
#box_demo {
    fn main() {
        window {
            width: 500
            height: 400
            title: "Box Demo"
            
            box {
                id: output
                x: 50
                y: 50
                width: 400
                height: 250
                text: "Output will appear here..."
            }
            
            button {
                x: 150
                y: 320
                width: 200
                height: 40
                text: "Add Content"
                
                int num = rnd(1, 100)
                print(num, output)
            }
        }
    }
}
```

---

## 相关文档

- [窗口系统高阶用法](./advanced/WindowSystemAdvanced.md)
- [内置函数](./Builtins.md)
- [基础语法](./BasicSyntax.md)
