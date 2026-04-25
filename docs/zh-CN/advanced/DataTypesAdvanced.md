# 数据类型高阶用法 (Data Types Advanced)

本文档介绍 xfawaPL 数据类型的高级技巧和组合用法。

---

## 数组高级操作

### 数组切片与范围

```xfawa
#array_slicing {
    fn main() {
        int arr = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
        
        int first_five = arr[0...4]
        int last_three = arr[7...9]
        int middle = arr[3...6]
        
        print("First 5 elements:")
        for i in first_five {
            print(i)
        }
        
        print("Last 3 elements:")
        for i in last_three {
            print(i)
        }
    }
}
```

### 负索引访问

```xfawa
#negative_index {
    fn main() {
        int arr = [10, 20, 30, 40, 50]
        
        print(arr[-1])    // 最后一个元素: 50
        print(arr[-2])    // 倒数第二个: 40
        print(arr[-3])    // 倒数第三个: 30
        
        int last_two = arr[-2...-1]
        for i in last_two {
            print(i)
        }
    }
}
```

### 动态数组构建

```xfawa
#dynamic_array {
    fn main() {
        fn build_array(start, end, step) {
            int result = []
            int current = start
            int i = 0
            
            while current <= end {
                result[i] = current
                current = current + step
                i = i + 1
            }
            return result
        }
        
        int evens = build_array(0, 20, 2)
        int odds = build_array(1, 19, 2)
        
        print("Even numbers:")
        for n in evens {
            print(n)
        }
        
        print("Odd numbers:")
        for n in odds {
            print(n)
        }
    }
}
```

---

## 多维数组模拟

### 二维数组实现

```xfawa
#matrix {
    fn main() {
        fn create_matrix(rows, cols, default) {
            int matrix = []
            int i = 0
            while i < rows * cols {
                matrix[i] = default
                i = i + 1
            }
            return matrix
        }
        
        fn get(matrix, row, col, cols) {
            return matrix[row * cols + col]
        }
        
        fn set(matrix, row, col, cols, value) {
            matrix[row * cols + col] = value
        }
        
        int rows = 3
        int cols = 4
        int mat = create_matrix(rows, cols, 0)
        
        set(mat, 0, 0, cols, 1)
        set(mat, 1, 1, cols, 5)
        set(mat, 2, 3, cols, 9)
        
        int r = 0
        while r < rows {
            int c = 0
            string row_str = ""
            while c < cols {
                row_str = row_str + get(mat, r, c, cols) + " "
                c = c + 1
            }
            print(row_str)
            r = r + 1
        }
    }
}
```

### 矩阵运算

```xfawa
#matrix_ops {
    fn main() {
        fn matrix_add(a, b, size) {
            int result = []
            int i = 0
            while i < size {
                result[i] = a[i] + b[i]
                i = i + 1
            }
            return result
        }
        
        fn matrix_scale(m, scalar, size) {
            int result = []
            int i = 0
            while i < size {
                result[i] = m[i] * scalar
                i = i + 1
            }
            return result
        }
        
        int mat_a = [1, 2, 3, 4]
        int mat_b = [5, 6, 7, 8]
        
        int sum = matrix_add(mat_a, mat_b, 4)
        int scaled = matrix_scale(mat_a, 2, 4)
        
        print("Sum:")
        for i in sum {
            print(i)
        }
        
        print("Scaled:")
        for i in scaled {
            print(i)
        }
    }
}
```

---

## 类型转换技巧

### 安全类型转换

```xfawa
#safe_conversion {
    fn main() {
        fn int_to_float_safe(n) {
            return n * 1.0
        }
        
        fn float_to_int_safe(f) {
            return f / 1
        }
        
        fn bool_to_int(b) {
            if b {
                return 1
            }
            return 0
        }
        
        fn int_to_bool(n) {
            return n != 0
        }
        
        int integer = 42
        float decimal = int_to_float_safe(integer)
        bool flag = int_to_bool(integer)
        
        print("Integer: " + integer)
        print("Float: " + decimal)
        print("Boolean: " + flag)
    }
}
```

### 字符串与数字转换

```xfawa
#string_conversion {
    fn main() {
        fn int_to_string(n) {
            return "" + n
        }
        
        fn format_percentage(value, total) {
            float percent = (value * 100.0) / total
            return int_to_string(value) + " / " + int_to_string(total) + " = " + percent + "%"
        }
        
        string result = format_percentage(25, 100)
        print(result)
        
        string formatted = "Score: " + 95 + " points"
        print(formatted)
    }
}
```

---

## 高级数据结构

### 栈实现

```xfawa
#stack {
    fn main() {
        int stack = []
        int top = 0
        
        fn push(value) {
            stack[top] = value
            top = top + 1
        }
        
        fn pop() {
            if top > 0 {
                top = top - 1
                return stack[top]
            }
            return -1
        }
        
        fn peek() {
            if top > 0 {
                return stack[top - 1]
            }
            return -1
        }
        
        fn is_empty() {
            return top == 0
        }
        
        push(10)
        push(20)
        push(30)
        
        print("Top: " + peek())
        print("Pop: " + pop())
        print("Pop: " + pop())
        print("Empty: " + is_empty())
    }
}
```

### 队列实现

```xfawa
#queue {
    fn main() {
        int queue = []
        int front = 0
        int rear = 0
        
        fn enqueue(value) {
            queue[rear] = value
            rear = rear + 1
        }
        
        fn dequeue() {
            if front < rear {
                int value = queue[front]
                front = front + 1
                return value
            }
            return -1
        }
        
        fn peek() {
            if front < rear {
                return queue[front]
            }
            return -1
        }
        
        fn size() {
            return rear - front
        }
        
        enqueue(1)
        enqueue(2)
        enqueue(3)
        
        print("Size: " + size())
        print("Dequeue: " + dequeue())
        print("Dequeue: " + dequeue())
        print("Size: " + size())
    }
}
```

### 链表模拟

```xfawa
#linked_list {
    fn main() {
        int values = []
        int nexts = []
        int head = -1
        int free_index = 0
        
        fn insert(value) {
            int new_node = free_index
            free_index = free_index + 1
            
            values[new_node] = value
            nexts[new_node] = head
            head = new_node
        }
        
        fn print_list() {
            int current = head
            print("List: ")
            while current != -1 {
                print(values[current])
                current = nexts[current]
            }
        }
        
        fn find(value) {
            int current = head
            while current != -1 {
                if values[current] == value {
                    return current
                }
                current = nexts[current]
            }
            return -1
        }
        
        insert(10)
        insert(20)
        insert(30)
        
        print_list()
        print("Found 20 at: " + find(20))
    }
}
```

---

## 数据处理模式

### 过滤器模式

```xfawa
#filter {
    fn main() {
        fn filter_positive(arr) {
            int result = []
            int j = 0
            int i = 0
            
            while i < len(arr) {
                if arr[i] > 0 {
                    result[j] = arr[i]
                    j = j + 1
                }
                i = i + 1
            }
            return result
        }
        
        fn filter_even(arr) {
            int result = []
            int j = 0
            int i = 0
            
            while i < len(arr) {
                if arr[i] % 2 == 0 {
                    result[j] = arr[i]
                    j = j + 1
                }
                i = i + 1
            }
            return result
        }
        
        int numbers = [-3, -1, 0, 2, 4, 6, 8, -5]
        int positive = filter_positive(numbers)
        int even = filter_even(numbers)
        
        print("Positive numbers:")
        for n in positive {
            print(n)
        }
        
        print("Even numbers:")
        for n in even {
            print(n)
        }
    }
}
```

### 映射模式

```xfawa
#map {
    fn main() {
        fn map_double(arr) {
            int result = []
            int i = 0
            while i < len(arr) {
                result[i] = arr[i] * 2
                i = i + 1
            }
            return result
        }
        
        fn map_square(arr) {
            int result = []
            int i = 0
            while i < len(arr) {
                result[i] = arr[i] * arr[i]
                i = i + 1
            }
            return result
        }
        
        int numbers = [1, 2, 3, 4, 5]
        int doubled = map_double(numbers)
        int squared = map_square(numbers)
        
        print("Doubled:")
        for n in doubled {
            print(n)
        }
        
        print("Squared:")
        for n in squared {
            print(n)
        }
    }
}
```

### 归约模式

```xfawa
#reduce {
    fn main() {
        fn reduce_sum(arr) {
            int total = 0
            int i = 0
            while i < len(arr) {
                total = total + arr[i]
                i = i + 1
            }
            return total
        }
        
        fn reduce_product(arr) {
            int total = 1
            int i = 0
            while i < len(arr) {
                total = total * arr[i]
                i = i + 1
            }
            return total
        }
        
        fn reduce_max(arr) {
            int maximum = arr[0]
            int i = 1
            while i < len(arr) {
                if arr[i] > maximum {
                    maximum = arr[i]
                }
                i = i + 1
            }
            return maximum
        }
        
        int numbers = [1, 2, 3, 4, 5]
        
        print("Sum: " + reduce_sum(numbers))
        print("Product: " + reduce_product(numbers))
        print("Max: " + reduce_max(numbers))
    }
}
```

---

## 组合示例

### 数据分析器

```xfawa
#data_analyzer {
    fn main() {
        fn analyze(arr) {
            int sum = 0
            int min_val = arr[0]
            int max_val = arr[0]
            int count = len(arr)
            int i = 0
            
            while i < count {
                sum = sum + arr[i]
                if arr[i] < min_val {
                    min_val = arr[i]
                }
                if arr[i] > max_val {
                    max_val = arr[i]
                }
                i = i + 1
            }
            
            float avg = sum * 1.0 / count
            
            print("=== Data Analysis ===")
            print("Count: " + count)
            print("Sum: " + sum)
            print("Average: " + avg)
            print("Min: " + min_val)
            print("Max: " + max_val)
            print("Range: " + (max_val - min_val))
        }
        
        int data = [12, 45, 67, 23, 89, 34, 56, 78, 90, 11]
        analyze(data)
    }
}
```

---

## 相关文档

- [数据类型](../DataTypes.md)
- [运算符高阶用法](./OperatorsAdvanced.md)
- [控制流高阶用法](./ControlFlowAdvanced.md)
