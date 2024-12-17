#define HL_NAME(n) heaps_##n
#include <meshoptimizer.h>
#include <hl.h>

HL_PRIM int HL_NAME(generate_vertex_remap)(unsigned int* pRemapOut, unsigned int* pIndices, int indexCount, float* pVertices, int vertexCount, int vertexSize) {
	return meshopt_generateVertexRemap(pRemapOut, pIndices, indexCount, pVertices, vertexCount, vertexSize);
}

HL_PRIM void HL_NAME(remap_index_buffer)(unsigned int* pIndicesOut, unsigned int* pIndicesIn, int indexCount, unsigned int* pRemap) {
	meshopt_remapIndexBuffer(pIndicesOut, pIndicesIn, indexCount, pRemap);
}

HL_PRIM void HL_NAME(remap_vertex_buffer)(void* pVerticesOut, void* pVerticexIn, int vertexCount, int vertexSize, unsigned int* pRemap) {
	meshopt_remapVertexBuffer(pVerticesOut, pVerticexIn, vertexCount, vertexSize, pRemap);
}

HL_PRIM int HL_NAME(simplify)(unsigned int* pIndicesOut, unsigned int* pIndicesIn, int indexCount, float* pVertices, int vertexCount, int vertexSize, int targetIndexCount, float targetError, int options, float* resultErrorOut) {
	return meshopt_simplify(pIndicesOut, pIndicesIn, indexCount, pVertices, vertexCount, vertexSize, targetIndexCount, (float)targetError, options, resultErrorOut);
}

HL_PRIM void HL_NAME(optimize_vertex_cache)(unsigned int* pIndicesOut, unsigned int* pIndicesIn, int indexCount, int vertexCount) {
	meshopt_optimizeVertexCache(pIndicesOut, pIndicesIn, indexCount, vertexCount);
}

HL_PRIM void HL_NAME(optimize_overdraw)(unsigned int* pIndicesOut, unsigned int* pIndicesIn, int indexCount, float* pVertices, int vertexCount, int vertexSize, float threshold) {
	meshopt_optimizeOverdraw(pIndicesOut, pIndicesIn, indexCount, pVertices, vertexCount, vertexSize, threshold);
}

HL_PRIM int HL_NAME(optimize_vertex_fetch)(float* pVerticesOut, unsigned int* pIndices, int indexCount, void* pVerticesIn, int vertexCount, int vertexSize) {
	return meshopt_optimizeVertexFetch(pVerticesOut, pIndices, indexCount, pVerticesIn, vertexCount, vertexSize);
}

DEFINE_PRIM(_I32, generate_vertex_remap, _BYTES _BYTES _I32 _BYTES _I32 _I32);
DEFINE_PRIM(_VOID, remap_index_buffer, _BYTES _BYTES _I32 _BYTES);
DEFINE_PRIM(_VOID, remap_vertex_buffer, _BYTES _BYTES _I32 _I32 _BYTES);
DEFINE_PRIM(_I32, simplify, _BYTES _BYTES _I32 _BYTES _I32 _I32 _I32 _F32 _I32 _BYTES);
DEFINE_PRIM(_VOID, optimize_vertex_cache, _BYTES _BYTES _I32 _I32);
DEFINE_PRIM(_VOID, optimize_overdraw, _BYTES _BYTES _I32 _BYTES _I32 _I32 _F32);
DEFINE_PRIM(_I32, optimize_vertex_fetch, _BYTES _BYTES _I32 _BYTES _I32 _I32);
