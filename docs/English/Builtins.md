# Built-in Functions

This document introduces built-in functions in xfawaPL.

---

## Output Function

### print

Output content to console or specified target.

#### Syntax

```xfawa
print(value)              // Output to console
print(value, box_id)      // Output to window component
```

#### Parameters

| Parameter | Type | Description |
|------|------|------|
| `value` | any | Value to output |
| `box_id` | string | Window box component ID |

#### Example

```xfawa
#print_demo {
    fn main() {
        print("Hello, World!")
        print(42)
        print(3.14)
        print(true)
        
        int arr = [1, 2, 3]
        print(arr)
    }
}
```

#### Output to Window Component

```xfawa
#window_print {
    fn main() {
        window {
            width: 400
            height: 300
            title: "Print Demo"
            
            box {
                id: output
                x: 50
                y: 50
                width: 300
                height: 200
                text: "Output here"
            }
            
            button {
                x: 150
                y: 260
                width: 100
                height: 30
                text: "Print"
                
                print("Button clicked!", output)
            }
        }
    }
}
```

---

## Random Function

### rnd

Generate random numbers or randomly select elements from an array.

#### Syntax

```xfawa
rnd(array)           // Randomly select an element from array
rnd(min, max)        // Generate random integer between min and max
```

#### Parameters

| Parameter | Type | Description |
|------|------|------|
| `array` | array | Array |
| `min` | int | Minimum value |
| `max` | int | Maximum value |

#### Return Value

- From array: Returns random element from array
- Range random: Returns random integer between min and max

#### Example

```xfawa
#rnd_demo {
    fn main() {
        int arr = [1, 2, 3, 4, 5]
        
        print("Random from array:")
        print(rnd(arr))
        
        print("Random 1-10:")
        print(rnd(1, 10))
        
        print("Random 1-100:")
        print(rnd(1, 100))
    }
}
```

---

## Related Documents

- [Built-in Functions Advanced Usage](./advanced/BuiltinsAdvanced.md)
- [Control Flow](./ControlFlow.md)
- [Window System](./WindowSystem.md)
