file(GLOB_RECURSE scripts_STAT_SRCS *.cpp *.h)

MACRO(ADD_MSVC_PRECOMPILED_HEADER PrecompiledHeader PrecompiledSource SourcesVar)
IF(MSVC)
GET_FILENAME_COMPONENT(PrecompiledBasename ${PrecompiledHeader} NAME_WE)
SET(PrecompiledBinary "${CMAKE_CURRENT_BINARY_DIR}/${PrecompiledBasename}.pch")
SET(Sources ${${SourcesVar}})
SET_SOURCE_FILES_PROPERTIES(${Sources}
    PROPERTIES COMPILE_FLAGS "/Yu\"${PrecompiledHeader}\" /FI\"${PrecompiledBinary}\" /Fp\"${PrecompiledBinary}\" /Zm120"
   OBJECT_DEPENDS "${PrecompiledBinary}")  
SET_SOURCE_FILES_PROPERTIES("include/${PrecompiledSource}"
PROPERTIES COMPILE_FLAGS "/Yc\"${PrecompiledHeader}\" /Fp\"${PrecompiledBinary}\" /Zm120"
OBJECT_OUTPUTS "${PrecompiledBinary}")
# Add precompiled header to SourcesVar
LIST(APPEND ${SourcesVar} "include/${PrecompiledSource}")
ENDIF(MSVC)
ENDMACRO(ADD_MSVC_PRECOMPILED_HEADER)

IF (WIN32 AND MSVC)
    ADD_MSVC_PRECOMPILED_HEADER("precompiled.h" "precompiled.cpp" scripts_STAT_SRCS)
ENDIF()
add_library(scripts STATIC ${scripts_STAT_SRCS})
ADD_DEPENDENCIES(game scripts revision.h)
