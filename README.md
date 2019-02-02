
The repo includes reflection library for C++17 and generator for it.

## About library
Reflection library features:
- No macro usage
- No injections into code
- Compact implementation (thanks to C++17)
- No global variables (reflected items are stored in Reflection object)
- Dynamic library friendly

Currently supported:
- Function
- Classes
- Constructor/destructor
- Allocation on stack/heap/inplace
- Methods (including pure inline)
- Pointers (adress-of and dereference operations support)

Planned:
- Inheritanse
- Enums

## Usage
Library is laying in `reflection/ ` folder. You can use pure sources from `src` dir, or connect it to your cmake project like this:
```
# Link reflection
add_subdirectory("../reflection" "MojitoReflection/")
target_link_libraries(${PROJECT_NAME} MojitoReflection)
```
Also see examples in `demo` and `tests` folders

## About generator
Generator works as a wrapper for a compiler and mimics compiler's behaviour. This works similar to ccache or emscripten. Generated reflection will be injected into the final binary (executable or shared library)

Generator is proved working CMake with XCode and Ninja generators. It also should be easily integrated with Qt Creator and CLion (not tested).

Features:
- C++ friendly and correct (thanks to clang in core)
- Reflected classes can be placed in cpp files or headers
- Attributes used to mark reflected classes

Limitations:
- Currently support only clang compiller (and possibly gcc, not tested)
- Can be easy built only for linux and mac, building for windows is possible but tricky 
- Requires support of chaning compiller from build system and sometimes from IDE


## Building generator
# MacOS
Example with usage brew
```
brew install llvm boost ninja cmake
cd generator
mkdir build
cd build
cmake -G "Ninja" ..
ninja
```
CMake may ask you to point to LLVM search path. If it does, feed him argument similar to this:
`-DLLVM_DIR=/usr/local/Cellar/llvm/5.0.0/lib/cmake/llvm`

# Linux
Install llvm >= 7, boost > 1.6, ninja, cmake with your package manager
```
cd generator
mkdir build
cd build
cmake -G "Ninja" ..
ninja
```

## Using generator
Honestly, this part is a bit treacky. Lets try just to run generator first. (Expected that you have built it with instruction above)
```
echo "int main() { }" > test.cpp
./reflection_generator --reflection-name=Experiment --compiller=c++ --reflection-includes="../../reflection/src/" --reflection-out=./reflection test.cpp
```
If everything done right, you will get a binary `unknown` in the folder.

As you see, a minimal pack of parameters for generator is:
- `--reflection-name` - Name of subfolder in `--reflection-out` folder
- `--compiller` - Compiller command
- `--reflection-includes` - Path to reflection library
- `--reflection-out` - Out folder

You can pass these parameters as additional compiller flags or make a wrapper with bash scripts as I did.

Okay I'm tired just look at examples in 'demo' and 'tests' folder. Both examples can be built with Xcode and Ninja generators. Like:
```
cd demo
mkdir build
cd build
cmake -G "XCode" .. 
```
