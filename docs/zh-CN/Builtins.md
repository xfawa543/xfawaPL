# 内置函数 (Built-in Functions)

本文档介绍 xfawaPL 的内置函数。

---

## 输出函数

### print

输出内容到控制台或指定目标。

#### 语法

```xfawa
print(value)              // 输出到控制台
print(value, box_id)      // 输出到窗口组件
```

#### 参数

| 参数 | 类型 | 说明 |
|------|------|------|
| `value` | 任意 | 要输出的值 |
| `box_id` | string | 窗口中的 box 组件 ID |

#### 示例

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

#### 输出到窗口组件

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

## 随机函数

### rnd

生成随机数或从数组中随机选择元素。

#### 语法

```xfawa
rnd(array)           // 从数组中随机选择一个元素
rnd(min, max)        // 生成 min 到 max 之间的随机整数
```

#### 参数

| 参数 | 类型 | 说明 |
|------|------|------|
| `array` | array | 数组 |
| `min` | int | 最小值 |
| `max` | int | 最大值 |

#### 返回值

- 从数组选择：返回数组中的随机元素
- 范围随机：返回 min 到 max 之间的随机整数

#### 示例

```xfawa
#rnd_demo {
    fn main() {
        int arr = [1, 2, 3, 4, 5]
        
        print("从数组随机选择:")
        print(rnd(arr))
        
        print("1到10随机数:")
        print(rnd(1, 10))
        
        print("1到100随机数:")
        print(rnd(1, 100))
    }
}
```

---

## 相关文档

- [内置函数高阶用法](./advanced/BuiltinsAdvanced.md)
- [控制流](./ControlFlow.md)
- [窗口系统](./WindowSystem.md)
