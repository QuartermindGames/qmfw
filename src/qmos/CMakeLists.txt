set(CMAKE_C_STANDARD 23)

option(QM_OS_MEMORY_BLOCK "Include memory block tracking." ON)

add_library(qm-os STATIC
        private/qm_os_library.c
        private/qm_os_linked_list.c
        private/qm_os_memory.c
        private/qm_os_random.c
        private/qm_os_shared_ptr.c
        private/qm_os_string.c
        private/qm_os_time.c

        public/qm_os.h
        public/qm_os_library.h
        public/qm_os_linked_list.h
        public/qm_os_memory.h
        public/qm_os_random.h
        public/qm_os_shared_ptr.h
        public/qm_os_string.h
        public/qm_os_time.h
)

if (QM_OS_MEMORY_BLOCK)
    target_compile_definitions(qm-os PUBLIC QM_OS_MEMORY_BLOCK)
endif ()

target_include_directories(qm-os PUBLIC ..)

#############################################
# Tests
#############################################
if (CMAKE_TESTING_ENABLED)
    add_executable(qm-os-test test/qm_os_test.c)
    target_link_libraries(qm-os-test qm-os)

    add_test(NAME QmTestOsApi COMMAND qm-os-test)
endif ()
