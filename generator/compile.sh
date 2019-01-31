#!/bin/sh
pushd $( dirname "${BASH_SOURCE[0]}")
mkdir build
pushd build
cmake -G "Ninja" ..
ninja
popd
popd
$( dirname "${BASH_SOURCE[0]}")/build/reflection_generator --reflection-includes $( dirname "${BASH_SOURCE[0]}")/../reflection/src/ --reflection-out ~/.mojito/reflection $@
