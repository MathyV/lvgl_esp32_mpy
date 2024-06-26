find_package(Python3 REQUIRED COMPONENTS Interpreter)

# Set some variables
set(LVGL_BINDINGS_DIR       ${CMAKE_CURRENT_LIST_DIR})                                  # Location of the binding
set(LVGL_ROOT_DIR           ${LVGL_BINDINGS_DIR}/lvgl)                                  # Location of LVGL

# Include the actual LVGL build, produces the lvgl_interface target
include(${LVGL_ROOT_DIR}/env_support/cmake/micropython.cmake)

# Set binding variables
set(LVGL_MPY                ${CMAKE_BINARY_DIR}/lv_mp.c)                                # Generated bindings
set(LVGL_MPY_PP             ${LVGL_MPY}.pp)                                             # Preprocessed bindings
set(LVGL_MPY_METADATA       ${LVGL_MPY}.json)                                           # Bindings metadata
set(LVGL_LVGL_H             ${LVGL_ROOT_DIR}/lvgl.h)                                    # lvgl.h
set(LVGL_GEN_MPY            ${LVGL_BINDINGS_DIR}/gen/gen_mpy.py)                        # gen_mpy.py script
set(LVGL_FAKE_LIBC          ${LVGL_BINDINGS_DIR}/pycparser/utils/fake_libc_include)     # Fake libc implementation

# Gather the headers
file(GLOB_RECURSE LVGL_HEADERS ${LVGL_ROOT_DIR}/src/*.h ${LVGL_BINDINGS_DIR}/lv_conf.h)

# Preprocess the bindings
add_custom_command(
    OUTPUT
        ${LVGL_MPY_PP}
    COMMAND
    ${CMAKE_C_COMPILER}
        -E
        -DPYCPARSER
        -I ${LVGL_FAKE_LIBC}
        ${MICROPY_CPP_FLAGS}
        ${LVGL_LVGL_H}
        > ${LVGL_MPY_PP}
    DEPENDS
        ${LVGL_LVGL_H}
        ${LVGL_HEADERS}
        ${LVGL_FAKE_LIBC}
    IMPLICIT_DEPENDS
        C ${LVGL_LVGL_H}
    VERBATIM
    COMMAND_EXPAND_LISTS
)

# Actually generate the bindings
add_custom_command(
    OUTPUT
        ${LVGL_MPY}
    COMMAND
        ${Python3_EXECUTABLE}
            ${LVGL_GEN_MPY}
            -M lvgl
            -MP lv
            -MD ${LVGL_MPY_METADATA}
            -E ${LVGL_MPY_PP}
            ${LVGL_LVGL_H}
            > ${LVGL_MPY} || (rm -f ${LVGL_MPY} && /bin/false)
    DEPENDS
        ${LVGL_GEN_MPY}
        ${LVGL_MPY_PP}
    COMMAND_EXPAND_LISTS
)

# Unfortunately IDF requires all files to be present during configuration, but these only get written during the
# build, so we temporarily write empty files so that IDF is happy
if (NOT EXISTS ${LVGL_MPY})
    file(WRITE ${LVGL_MPY} "")
endif ()

add_library(usermod_lv_bindings INTERFACE)
target_sources(usermod_lv_bindings INTERFACE ${LVGL_MPY})
target_include_directories(usermod_lv_bindings INTERFACE ${LVGL_BINDINGS_DIR})

target_link_libraries(usermod_lv_bindings INTERFACE lvgl_interface)

# make usermod (target declared by Micropython for all user compiled modules) link to bindings
# this way the bindings (and transitively lvgl_interface) get proper compilation flags
target_link_libraries(usermod INTERFACE usermod_lv_bindings)
