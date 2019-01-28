#!/bin/sh
echo $@
$( dirname "${BASH_SOURCE[0]}")/../generator/compile.sh --reflection-name MojitoDemo --reflection-includes ../reflection/src/ --compiller $CXX $@
