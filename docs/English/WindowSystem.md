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

## XSS Style System

Use XSS (xfawa Style Sheets) files to define component styles.

### Syntax

Specify style file in window:

```xfawa
window {
    width: 600
    height: 400
    title: "Styled Window"
    color: white
    style: "styles.xss"

    // components...
}
```

### XSS File Format

```css
button {
    background-color: #2d7ff9
    text-color: white
    border-radius: 8
    border-width: 1
    border-color: #1d4fa0
}
```

### Style Properties

| Property | Type | Description |
|------|------|------|
| `background-color` | color | Background color (hexadecimal) |
| `text-color` | color | Text color |
| `border-radius` | int | Border radius |
| `border-width` | int | Border width |
| `border-color` | color | Border color |

---

## Animation System

Define component animation effects in XSS files.

### Animation Parameters

| Parameter | Description | Example |
|------|------|------|
| `x-animation` | X-axis movement animation | `x-animation: animation(10, 200)` |
| `y-animation` | Y-axis movement animation | `y-animation: animation(50, 150)` |
| `width-animation` | Width change animation | `width-animation: animation(100, 300)` |
| `height-animation` | Height change animation | `height-animation: animation(40, 80)` |

### Animation Syntax

```css
animation(initial_value, target_value)
```

- **initial_value**: Value at animation start (pixels)
- **target_value**: Value at animation end (pixels)

### Animation Triggers

Use `animation-trigger {}` sub-block to define animation trigger conditions. Only animation parameters defined inside the trigger block will be controlled by the trigger.

#### Trigger Syntax

```css
button {
    // style properties...

    animation-trigger {
        // animation parameters (inside trigger block)
        x-animation: animation(10, 200)
        y-animation: animation(50, 150)
        width-animation: animation(100, 300)
        height-animation: animation(40, 80)

        // trigger conditions
        window-click()           // full window click trigger
        window-click(x, y, w, h) // specific area click trigger
        key-click: "A"           // key press trigger
        button-click: "namespace" // button namespace trigger
    }
}
```

#### Trigger Condition Types

| Trigger | Syntax | Description |
|--------|------|------|
| **window-click** | `window-click()` | Trigger on click anywhere in window |
| **window-click** | `window-click(x, y, w, h)` | Trigger on click in specific area (x, y is top-left corner, w, h is area width and height) |
| **key-click** | `key-click: "key_name"` | Trigger on key press (supports A-Z letter keys) |
| **button-click** | `button-click: "namespace"` | Trigger on click of button with specified namespace |

### Animation Examples

#### Example 1: Window Click Trigger

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

#### Example 2: Key Click Trigger

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

### Animation Behavior Notes

1. **Auto-play Control**: Animation parameters inside `animation-trigger {}` block won't auto-play on program startup
2. **Trigger Activation**: Animation only activates and starts executing when trigger condition is met
3. **Animation Order**: Multiple animation parameters execute simultaneously, linearly interpolating from initial value to target value
4. **Unit**: All animation values are in pixels

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
