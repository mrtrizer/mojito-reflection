#pragma once

#define reflect __attribute((annotate("reflect")))
#define propset(name) __attribute((annotate("property_set:" ## name)))
#define propget(name) __attribute((annotate("property_get:" ## name)))
#define hide __attribute((annotate("function")))
