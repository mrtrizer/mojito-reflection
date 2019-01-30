#pragma once

#include "Function.hpp"

namespace mojito {

struct Constructor {
    Function onStackConstructor;
    Function onHeapConstructor;
    Function inAddressConstructor;
};

} // mojito
