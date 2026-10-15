set(CMAKE_C_STANDARD 23)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

option(QM_GFX_OPENGL "Include OpenGL support." ON)
if (QM_GFX_OPENGL)
    set(QM_GFX_OPENGL_SOURCES
            private/driver_opengl/3rdparty/glew-2.2.0/src/glew.c
            private/driver_opengl/xgl.c
            private/driver_opengl/xgl_mesh.c
    )
    set(QM_GFX_DEFINITIONS ${QM_GFX_DEFINITIONS}
            QM_GFX_OPENGL=1
            GLEW_STATIC
            GLEW_NO_GLU
    )
    set(QM_GFX_INCLUDES ${QM_GFX_INCLUDES}
            private/driver_opengl/3rdparty/glew-2.2.0/include/
    )
    if (WIN32)
        set(QM_GFX_LIBRARIES ${QM_GFX_LIBRARIES} OpenGL32)
    else ()
        set(QM_GFX_LIBRARIES ${QM_GFX_LIBRARIES} GL)
    endif ()
endif ()

option(QM_GFX_VULKAN "Include Vulkan support." OFF)

add_library(plgraphics STATIC
        private/plg_draw.c
        private/plg_texture.c

        private/qm_gfx.c
        private/qm_gfx_driver.c
        private/qm_gfx_framebuffer.c
        private/qm_gfx_mesh.c
        private/qm_gfx_shader.c

        ${QM_GFX_OPENGL_SOURCES}
)

target_compile_definitions(plgraphics PRIVATE ${QM_GFX_DEFINITIONS})
target_include_directories(plgraphics
        PUBLIC include/
        PRIVATE ${QM_GFX_INCLUDES}
)
target_link_libraries(plgraphics plcore ${QM_GFX_LIBRARIES})
