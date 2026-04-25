# Window System

This document introduces the xfawaPL window system (Xraphics).

---

## window Statement

Create native window applications.

### Syntax

```xfawa
window {
    width: <width>
    height: <height>
    title: "<title>"
    color: <color>
}
```

### Window Properties

| Property | Type | Description | Example |
|------|------|------|------|
| `width` | int | Window width | `width: 400` |
| `height` / `high` | int | Window height | `height: 300` |
| `title` | string | Window title | `title: "My App"` |
| `color` | string | Window background color | `color: red` |
| `style` | string | Window style | `style: modern` |

### Supported Colors

| Color Name | Description |
|--------|------|
| `red` | Red |
| `green` | Green |
| `blue` | Blue |
| `white` | White |
| `black` | Black |
| `yellow` | Yellow |
| `cyan` | Cyan |
| `lightblue` | Light blue |
| `lightgreen` | Light green |

### Example

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

## text Component

Add text display to window.

### Syntax

```xfawa
text {
    x: <X coordinate>
    y: <Y coordinate>
    width: <width>
    height: <height>
    text: "<text content>"
}
```

### Properties

| Property | Type | Description |
|------|------|------|
| `x` | int | X coordinate |
| `y` | int | Y coordinate |
| `width` | int | Text area width |
| `height` | int | Text area height |
| `text` | string | Text to display |

### Example

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

## button Component

Add button to window.

### Syntax

```xfawa
button {
    x: <X coordinate>
    y: <Y coordinate>
    width: <width>
    height: <height>
    text: "<button text>"
    
    // Code to execute when button is clicked
    <code>
}
```

### Properties

| Property | Type | Description |
|------|------|------|
| `x` | int | X coordinate |
| `y` | int | Y coordinate |
| `width` | int | Button width |
| `height` / `high` | int | Button height |
| `text` / `title` | string | Button text |

### Example

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

## box Component

Add updatable text box to window, supports scrolling.

### Syntax

```xfawa
box {
    id: <identifier>
    x: <X coordinate>
    y: <Y coordinate>
    width: <width>
    height: <height>
    text: "<initial text>"
}
```

### Properties

| Property | Type | Description |
|------|------|------|
| `id` | string | Unique identifier for box |
| `x` | int | X coordinate |
| `y` | int | Y coordinate |
| `width` | int | Box width |
| `height` | int | Box height |
| `text` | string | Initial text content |

### Update box Content

Use `print` function to update box content:

```xfawa
print("new content", box_id)
```

### Scroll Feature

When box content exceeds visible area, use mouse wheel to scroll up and down.

### Example

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

## Related Documents

- [Window System Advanced Usage](./advanced/WindowSystemAdvanced.md)
- [Built-in Functions](./Builtins.md)
- [Basic Syntax](./BasicSyntax.md)
