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
