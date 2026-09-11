set(CMAKE_C_STANDARD 23)

add_library(qm-fs STATIC
        private/qm_fs.c
)

target_include_directories(qm-fs PUBLIC ..)

target_link_libraries(qm-fs qm-os)

#############################################
# Tests
#############################################
if (CMAKE_TESTING_ENABLED)
    add_executable(qm-fs-test test/qm_fs_test.c)
    # !!plcore link here is temporary until refactoring is completed!!
    target_link_libraries(qm-fs-test qm-fs plcore)

    add_test(NAME QmTestFsApi COMMAND qm-fs-test)
endif()
