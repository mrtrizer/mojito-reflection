#!/bin/sh
$( dirname "${BASH_SOURCE[0]}")/../generator/compile.sh --reflection-name MojitoTests --reflection-includes $( dirname "${BASH_SOURCE[0]}")/../reflection/src/ --compiller "c++" $@
