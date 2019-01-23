#!/bin/sh
mkdir -p ../generator/build
pushd ../generator/build
cmake -G "Ninja" ..
ninja
popd
../generator/build/reflection_generator --reflection-name Test --reflection-out ./reflection --reflection-includes ../reflection/src/ --compiller $CXX $@
