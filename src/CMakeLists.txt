#################################
# Modules                       #
#################################

include_directories(${CMAKE_SOURCE_DIR}/src)
add_subdirectory (util)
add_subdirectory (goto-programs)
add_subdirectory (c2goto)

if(ENABLE_OLD_FRONTEND)
add_subdirectory (ansi-c)
add_subdirectory (cpp)
endif()

if(ENABLE_SOLIDITY_FRONTEND)
add_subdirectory (solidity-frontend)
endif()

add_subdirectory (clang-c-frontend)
add_subdirectory (pointer-analysis)
add_subdirectory (langapi)
add_subdirectory (big-int)
add_subdirectory (solvers)
add_subdirectory (clang-cpp-frontend)
add_subdirectory (goto-symex)
add_subdirectory (esbmc)

set(ESBMC_AVAILABLE_SOLVERS "${ESBMC_AVAILABLE_SOLVERS}" PARENT_SCOPE)
