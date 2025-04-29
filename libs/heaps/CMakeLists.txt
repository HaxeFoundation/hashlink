set(HEAPS_INCLUDE_DIRS
    ${INCLUDES_BASE_DIR}/mikktspace
    ${INCLUDES_BASE_DIR}/meshoptimizer
    ${INCLUDES_BASE_DIR}/vhacd
    .
)

add_library(heaps.hdll SHARED
    ${INCLUDES_BASE_DIR}/mikktspace/mikktspace.c
    ${INCLUDES_BASE_DIR}/meshoptimizer/allocator.cpp
    ${INCLUDES_BASE_DIR}/meshoptimizer/overdrawoptimizer.cpp
	${INCLUDES_BASE_DIR}/meshoptimizer/vcacheoptimizer.cpp
    ${INCLUDES_BASE_DIR}/meshoptimizer/clusterizer.cpp
	${INCLUDES_BASE_DIR}/meshoptimizer/quantization.cpp
    ${INCLUDES_BASE_DIR}/meshoptimizer/vertexcodec.cpp
	${INCLUDES_BASE_DIR}/meshoptimizer/indexcodec.cpp
    ${INCLUDES_BASE_DIR}/meshoptimizer/simplifier.cpp
	${INCLUDES_BASE_DIR}/meshoptimizer/vertexfilter.cpp
    ${INCLUDES_BASE_DIR}/meshoptimizer/indexgenerator.cpp
	${INCLUDES_BASE_DIR}/meshoptimizer/spatialorder.cpp
    ${INCLUDES_BASE_DIR}/meshoptimizer/vfetchanalyzer.cpp
	${INCLUDES_BASE_DIR}/meshoptimizer/stripifier.cpp
    ${INCLUDES_BASE_DIR}/meshoptimizer/vfetchoptimizer.cpp
	${INCLUDES_BASE_DIR}/meshoptimizer/overdrawanalyzer.cpp
    ${INCLUDES_BASE_DIR}/meshoptimizer/vcacheanalyzer.cpp
    mikkt.c
    meshoptimizer.cpp
    vhacd.cpp
)

set_as_hdll(heaps)

target_include_directories(heaps.hdll
    PRIVATE
    ${HEAPS_INCLUDE_DIRS}
)

target_link_libraries(heaps.hdll
    libhl
)

install(
    TARGETS
        heaps.hdll
    DESTINATION ${HDLL_DESTINATION}
)
