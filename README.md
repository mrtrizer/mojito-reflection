[![Build Status](https://travis-ci.org/mrtrizer/mojito-reflection.svg?branch=master)](https://travis-ci.org/mrtrizer/mojito-reflection)

# C++ Reflection generator

The repo includes a reflection library for C++17 and a generator for it.
The root idea was to make reflection generation transparent by replacing the compiler with a reflection generator, that calls the compiler under the hood. This was the main concept and it comes with a huge drawback, the generator is hard to integrate into the project. So, I stopped developing once I got this issue. 
Maybe it could become more universal if I made more effort.

## About library
Reflection library features:
- No macro usage
- No injections into code
- Compact implementation (thanks to C++17)
- No global variables (reflected items are stored in the Reflection object)
- Dynamic library friendly

Currently supported:
- Function
- Classes
- Constructor/destructor
- Allocation on stack/heap/inplace
- Methods (including pure inline)
- Pointers (adress-of and dereference operations support)

Planned:
- Inheritance
- Enums

## Usage
The library is laying in `reflection/ ` folder. You can use pure sources from `src` dir, or connect it to your cmake project like this:
```
# Link reflection
add_subdirectory("../reflection" "MojitoReflection/")
target_link_libraries(${PROJECT_NAME} MojitoReflection)
```
Also, see examples in `demo` and `tests` folders

## About generator
The generator works as a wrapper for a compiler and mimics the compiler's behavior. This works similarly to ccache or emscripten. Generated reflection will be injected into the final binary (executable or shared library)

The generator is proven to work CMake with XCode and Ninja generators. It also should be easily integrated with Qt Creator and CLion (not tested).

Features:
- C++ friendly and correct (thanks to clang in core)
- Reflected classes can be placed in cpp files or headers
- Attributes used to mark reflected classes

Limitations:
- Currently support only clang compiler (and possibly gcc, not tested)
- Can be easily built only for Linux and Mac, building for Windows is possible but tricky 
- Requires support of changing compiler from build system and sometimes from IDE


## Building generator
### MacOS
Example with usage brew
```
brew install llvm boost ninja cmake
cd generator
mkdir build
cd build
cmake -G "Ninja" ..
ninja
```
CMake may ask you to point to LLVM search path. If it does, feed him an argument similar to this:
`-DLLVM_DIR=/usr/local/Cellar/llvm/5.0.0/lib/cmake/llvm`

### Linux
Install llvm >= 7, boost > 1.6, ninja, cmake with your package manager
```
cd generator
mkdir build
cd build
cmake -G "Ninja" ..
ninja
```

## Using generator
Honestly, this part is a bit tricky. Let's try just to run the generator first. (Expected that you have built it with the instructions above)
```
echo "int main() { }" > test.cpp
./reflection_generator --reflection-name=Experiment --compiller=c++ --reflection-includes="../../reflection/src/" --reflection-out=./reflection test.cpp
```
You will get a binary `unknown` in the folder if everything is done right.

As you see, a minimal pack of parameters for the generator is:
- `--reflection-name` - Name of subfolder in `--reflection-out` folder
- `--compiller` - Compiller command
- `--reflection-includes` - Path to reflection library
- `--reflection-out` - Out folder

You can pass these parameters as additional compiler flags or make a wrapper with bash scripts as I did.

Examples can be built with Xcode and Ninja generators. Like:
```
cd demo
mkdir build
cd build
cmake -G "XCode" .. 
```
