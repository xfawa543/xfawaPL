LLVM backend prototype

版本号：1.0.0-a.3
版本称号：Mod试炼

Version: 1.0.0-a.3
Version Title: The Mod Trial

This folder contains a minimal C++ prototype for the future LLVM backend.
It is intentionally tiny: it parses the first top-level block of an `.xf` file
and extracts `print("...")` statements, emits a temporary C++ program and
compiles it with `clang++` (preferred) or `g++` (fallback).

Usage (PowerShell):

    .\xfawac_llvm.exe ..\test\hello.xf -o hello_from_llvm.exe

Notes:
- This is a prototype for quick iteration; it is NOT a real LLVM IR emitter.
- To turn this into a real LLVM backend we will replace the code generation
  with LLVM IR builder calls (libLLVM / LLVM C++ APIs).
