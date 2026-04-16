# xfawa Programming Language

xfawa is an experimental compiled programming language project currently under development, aiming to explore:

- A flexible and extensible syntax design  
- High performance execution (based on LLVM)  
- Extensible syntax system (mods)  
- Lightweight runtime design  

The project is still in early alpha stage.

---

## 🚀 Project Goals

xfawa is NOT a scripting language. It is a compiled programming language experiment.

### 1. Compiled Language Design
xfawa is designed as a compiled language, where source code is translated into native machine code instead of being interpreted.

### 2. Syntax Exploration
xfawa aims to design a readable and flexible syntax, not bound to any existing language model.

### 3. High Performance Execution
Compile xfawa into native machine code using LLVM backend.

### 4. mods System (Core Feature)
The mods system allows syntax-level transformations before compilation.

It enables:

- Syntax extension  
- Experimental language features  
- Custom syntax behavior  

### 5. Lightweight Runtime
Minimize external dependencies by integrating standard library into the compiler distribution when possible.

---

## 🧪 Syntax Examples (Experimental)

These examples represent current design ideas and may change in the future.

### ✔ Variable Declaration
```
let x = 10
let name = "xfawa"
```

### ✔ Function Definition
```
fn add(a, b) {
    return a + b
}
```

### ✔ Conditional Statement
```
if x > 10 {
    print("large")
} else {
    print("small")
}
```

### ✔ Loop
```
for i in 0..10 {
    print(i)
}
```

---

## 🧩 mods System

mods is a core mechanism of xfawa, used to transform source code at early compilation stage.

It enables:

- Syntax extensions  
- Syntax sugar implementation  
- Experimental language features  

Design goal:

> Extend the language without modifying the compiler core.

---

## 📦 Dependencies

xfawa currently depends on:

- LLVM  
- System toolchain (Windows / Linux)  

Note:
The project aims to reduce unnecessary dependencies but does not avoid system-level toolchains.

---

## 👤 Founder’s Note

xfawa started as a compiler experiment rather than a fully defined language design.

The mods system gradually emerged during development rather than being fully designed from the beginning.

The project is still in a very early stage and may evolve significantly over time. However, one core idea remains:

> A programming language should be compiled and extensible, not a fixed set of rules.

---

## ⚠️ About Implementation

Some parts of this project were developed with the assistance of AI tools.

However, the overall architecture, design decisions, and direction are determined by the author.

---

## 📌 Current Status

- Early alpha  
- Compiled language  
- Syntax system under active development  
- mods system evolving  
- IR and backend under iteration  

---

## 📜 License

MIT Licensed - see [LICENSE](LICENSE) for details.