# Design project

## How to compile
```
flex tma3.l
bison -y -d tma3.y
gcc -c .\lex.yy.c .\y.tab.c
gcc .\lex.yy.c .\y.tab.c .\symbols.c .\symbol_table.c .\semantic.c .\parser.c .\ast.c .\stack.c .\codegen.c .\isa2.c -o .\tma3.exe

```

## Structure
1. [Register Allocation/Deallocation](#register-allocation-deallocation-scheme)
2. [Memory Usage](#memory-usage-scheme-stack-based)
3. [Code Generation Phases](#code-generation-phases)
4. [Intermediate Code Generator](#intermediate-code-generator)
5. [Symbol Table Integration](#symbol-table-integration)
6. [Target Code Generator (ISA2)](#target-code-generator-isa2)

---

## 1. Register Allocation/Deallocation Scheme

### Overview
The main objective of the design project is the create the intermediate code generator and the target code generator for accumulator based ISA

---

## 2. Memory Usage

---

## 3. Code Generation Phases

### Phase Overview

```
┌────────────────────────────────────────────────────────────┐
│                    INPUT: Verified AST                     │
│              (from Semantic Analysis Phase)                │
└────────────────────┬───────────────────────────────────────┘
                     │
                     ▼
┌────────────────────────────────────────────────────────────┐
│  PHASE 1: AST Traversal & 3AC Generation                   │
│  ───────────────────────────────────────                   │
│  Module: codegen.c                                         │
│  Entry:  generate_ir(ASTNode* root)                        │
│                                                            │
│  Actions:                                                  │
│  • Traverse AST recursively (traverse_all)                 │
│  • Generate quadruples for each construct                  │
│  • Manage temporary variables & labels                     │
│  • Build intermediate representation                       │
└────────────────────┬───────────────────────────────────────┘
                     │
                     ▼

```

---

## 4. Intermediate Code Generator

### 4.1 Integration with AST and Symbol Table

The intermediate code generator operates on a **semantically verified AST** with an **annotated symbol table**.

#### 4.1.1 Input Requirements

**From Semantic Analysis Phase**:
1. **Verified AST**: No semantic errors
2. **Populated Symbol Table**: All symbols declared, types resolved
3. **Offset Information**: Local/parameter offsets calculated
4. **Type Information**: Expression types determined

**Preconditions Verified** (semantic.c):
- All identifiers declared before use
- Type compatibility in assignments
- Function signatures match calls
- Return types correct
- Array bounds are constant

---

## 5. Symbol Table

---

## 6. Target Code Generator (ISA2)

<<<<<<< HEAD
=======
<<<<<<< HEAD
![alt text](image.png)

![alt text](image-1.png)

## Full Paper

=======
![image](https://github.com/user-attachments/assets/432a79e1-1166-48f0-9547-34c57da149b7)
![image](https://github.com/user-attachments/assets/0084e777-2f9e-4fa7-9923-f6e44a672db0)
>>>>>>> 5a5a9478e54e9e8d0647340a58c191508bd6706a
>>>>>>> 6126eaf1e3f0503570b925582e1bfc4ac2c04636
