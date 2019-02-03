#!/bin/sh
echo Compiller: $MOJITO_CC
$( dirname "${BASH_SOURCE[0]}")/compile.sh --compiller $MOJITO_CC $@
