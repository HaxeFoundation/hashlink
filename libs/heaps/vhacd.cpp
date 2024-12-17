#define HL_NAME(n) heaps_##n
#define ENABLE_VHACD_IMPLEMENTATION 1

#include <VHACD.h>
#include <hl.h>

typedef struct {
	VHACD::IVHACD* pInstance;
	std::vector<VHACD::IVHACD::ConvexHull> convexHulls;
} vhacd;

struct convex_hull {
	vbyte* points;
	vbyte* triangles;
	int pointCount;
	int triangleCount;
	double volume;
	double centerX;
	double centerY;
	double centerZ;
	int meshId;
	double boundsMinX;
	double boundsMinY;
	double boundsMinZ;
	double boundsMaxX;
	double boundsMaxY;
	double boundsMaxZ;
};

HL_PRIM vhacd* HL_NAME(create_vhacd)() {
	return new vhacd{ VHACD::CreateVHACD() };
}

HL_PRIM bool HL_NAME(vhacd_compute)(vhacd* pVhacd, float* pPoints, uint32_t countPoints, uint32_t* pTriangles, uint32_t countTriangle, VHACD::IVHACD::Parameters* pParameters) {
	return pVhacd->pInstance->Compute(pPoints, countPoints, pTriangles, countTriangle, *pParameters);
}

HL_PRIM int HL_NAME(vhacd_get_n_convex_hulls)(vhacd* pVhacd) {
	return pVhacd->pInstance->GetNConvexHulls();
}

HL_PRIM bool HL_NAME(vhacd_get_convex_hull)(vhacd* pVhacd, int index, convex_hull* pConvexHull) {
	pVhacd->convexHulls.emplace_back();
	VHACD::IVHACD::ConvexHull& convexHull = pVhacd->convexHulls.back();
	if (!pVhacd->pInstance->GetConvexHull(index, convexHull))
		return false;

	pConvexHull->points = (vbyte*)convexHull.m_points.data();
	pConvexHull->pointCount = (int)convexHull.m_points.size();
	pConvexHull->triangles = (vbyte*)convexHull.m_triangles.data();
	pConvexHull->triangleCount = (int)convexHull.m_triangles.size();
	pConvexHull->volume = convexHull.m_volume;
	pConvexHull->centerX = convexHull.m_center[0];
	pConvexHull->centerY = convexHull.m_center[1];
	pConvexHull->centerZ = convexHull.m_center[2];
	pConvexHull->meshId = convexHull.m_meshId;
	pConvexHull->boundsMinX = convexHull.mBmin[0];
	pConvexHull->boundsMinY = convexHull.mBmin[1];
	pConvexHull->boundsMinZ = convexHull.mBmin[2];
	pConvexHull->boundsMaxX = convexHull.mBmax[0];
	pConvexHull->boundsMaxY = convexHull.mBmax[1];
	pConvexHull->boundsMaxZ = convexHull.mBmax[2];

	return true;
}

HL_PRIM void HL_NAME(vhacd_clean)(vhacd* pVhacd) {
	pVhacd->pInstance->Clean();
}

HL_PRIM void HL_NAME(vhacd_release)(vhacd* pVhacd) {
	pVhacd->pInstance->Release();
	delete pVhacd;
}

#define _VHACD _ABSTRACT(vhacd)
DEFINE_PRIM(_VHACD, create_vhacd, _NO_ARG);
DEFINE_PRIM(_BOOL, vhacd_compute, _VHACD _BYTES _I32 _BYTES _I32 _STRUCT);
DEFINE_PRIM(_I32, vhacd_get_n_convex_hulls, _VHACD);
DEFINE_PRIM(_BOOL, vhacd_get_convex_hull, _VHACD _I32 _STRUCT);
DEFINE_PRIM(_VOID, vhacd_clean, _VHACD);
DEFINE_PRIM(_VOID, vhacd_release, _VHACD);
