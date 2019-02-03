#!/bin/sh
echo Compiller: $MOJITO_CXX
$( dirname "${BASH_SOURCE[0]}")/compile.sh --compiller $MOJITO_CXX $@
