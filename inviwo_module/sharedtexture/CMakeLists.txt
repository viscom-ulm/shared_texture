ivw_module(SharedTexture)

set(HEADER_FILES
    include/ulm_university/sharedtexture/processors/sharedtexturecanvas.h
    include/ulm_university/sharedtexture/sharedtexturemodule.h
    include/ulm_university/sharedtexture/sharedtexturemoduledefine.h
)
ivw_group("Header Files" ${HEADER_FILES})

set(SOURCE_FILES
    src/processors/sharedtexturecanvas.cpp
    src/sharedtexturemodule.cpp
)
ivw_group("Source Files" ${SOURCE_FILES})

set(SHADER_FILES
	# Add shaders
)
ivw_group("Shader Files" ${SHADER_FILES})

set(TEST_FILES
    tests/unittests/sharedtexture-unittest-main.cpp
)
ivw_add_unittest(${TEST_FILES})

ivw_create_module(${SOURCE_FILES} ${HEADER_FILES} ${SHADER_FILES})

# Add shader directory to install package
#ivw_add_to_module_pack(${CMAKE_CURRENT_SOURCE_DIR}/glsl)
