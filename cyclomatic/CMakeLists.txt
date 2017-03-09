cmake_minimum_required(VERSION 3.5.1)

project("CyclomaticPass")

add_library(CyclomaticPass MODULE
    # List your source files here.
        CyclomaticA.cpp
)


# LLVM is (typically) built with no C++ RTTI. We need to match that;
# otherwise, we'll get linker errors about missing RTTI data.
set_target_properties(CyclomaticPass PROPERTIES
    COMPILE_FLAGS "-fno-rtti"
)

# Get proper shared-library behavior (where symbols are not necessarily
# resolved when the shared library is linked) on OS X.
if(APPLE)
    set_target_properties(CyclomaticPass PROPERTIES
        LINK_FLAGS "-undefined dynamic_lookup"
    )
endif(APPLE)


