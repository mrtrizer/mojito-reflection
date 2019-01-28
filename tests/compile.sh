#!/bin/sh
pushd "$( dirname "${BASH_SOURCE[0]}")"
../generator/compile.sh --reflection-name MojitoTests --reflection-includes ../reflection/src/ --compiller $CXX $@
popd
