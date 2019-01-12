#include </Users/deniszdorovtsov/Projects/NewReflectionGenerator/tests/test.cpp>
#include <Type.hpp>
#include <BasicTypesReflection.hpp>

namespace flappy {
void registerTest(Reflection& reflection) {
  reflection.registerType<Test>("Test")
.addFunction("a", &Test::a)
.addConstructor<Test>()
;
} 
} 

