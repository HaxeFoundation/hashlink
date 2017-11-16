#define HL_NAME(x) bullet_##x
#include <hl.h>
#define _IDL _BYTES
template <typename T> struct pref {
	void *finalize;
	T *value;
};

#define _ref(t) pref<t>
#define _unref(v) v->value
#define alloc_ref_const(r, _) _alloc_const(r)
#define free_ref(v) delete _unref(v)

template<typename T> pref<T> *alloc_ref( T *value, void (*finalize)( pref<T> * ) ) {
	pref<T> *r = (pref<T>*)hl_gc_alloc_finalizer(sizeof(r));
	r->finalize = finalize;
	r->value = value;
	return r;
}

template<typename T> pref<T> *_alloc_const( const T *value ) {
	pref<T> *r = (pref<T>*)hl_gc_alloc_noptr(sizeof(r));
	r->finalize = NULL;
	r->value = (T*)value;
	return r;
}

#ifdef _WIN32
#pragma warning(disable:4305)
#pragma warning(disable:4244)
#pragma warning(disable:4316)
#endif
#include <btBulletDynamicsCommon.h>
#include <BulletSoftBody/btSoftBody.h>
#include <BulletSoftBody/btSoftBodyRigidBodyCollisionConfiguration.h>
#include <BulletSoftBody/btDefaultSoftBodySolver.h>
#include <BulletSoftBody/btSoftBodyHelpers.h>
#include <BulletSoftBody/btSoftRigidDynamicsWorld.h>
#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <BulletDynamics/Character/btKinematicCharacterController.h>

static void finalize_btVector3( _ref(btVector3)* _this ) { free_ref(_this); }
static void finalize_btVector4( _ref(btVector4)* _this ) { free_ref(_this); }
static void finalize_btQuadWord( _ref(btQuadWord)* _this ) { free_ref(_this); }
static void finalize_btQuaternion( _ref(btQuaternion)* _this ) { free_ref(_this); }
static void finalize_btMatrix3x3( _ref(btMatrix3x3)* _this ) { free_ref(_this); }
static void finalize_btTransform( _ref(btTransform)* _this ) { free_ref(_this); }
static void finalize_btMotionState( _ref(btMotionState)* _this ) { free_ref(_this); }
static void finalize_btDefaultMotionState( _ref(btDefaultMotionState)* _this ) { free_ref(_this); }
static void finalize_btCollisionObject( _ref(btCollisionObject)* _this ) { free_ref(_this); }
static void finalize_RayResultCallback( _ref(btCollisionWorld::RayResultCallback)* _this ) { free_ref(_this); }
static void finalize_ClosestRayResultCallback( _ref(btCollisionWorld::ClosestRayResultCallback)* _this ) { free_ref(_this); }
static void finalize_btManifoldPoint( _ref(btManifoldPoint)* _this ) { free_ref(_this); }
static void finalize_ContactResultCallback( _ref(btCollisionWorld::ContactResultCallback)* _this ) { free_ref(_this); }
static void finalize_LocalShapeInfo( _ref(btCollisionWorld::LocalShapeInfo)* _this ) { free_ref(_this); }
static void finalize_LocalConvexResult( _ref(btCollisionWorld::LocalConvexResult)* _this ) { free_ref(_this); }
static void finalize_ConvexResultCallback( _ref(btCollisionWorld::ConvexResultCallback)* _this ) { free_ref(_this); }
static void finalize_ClosestConvexResultCallback( _ref(btCollisionWorld::ClosestConvexResultCallback)* _this ) { free_ref(_this); }
static void finalize_btCollisionShape( _ref(btCollisionShape)* _this ) { free_ref(_this); }
static void finalize_btConvexShape( _ref(btConvexShape)* _this ) { free_ref(_this); }
static void finalize_btConvexTriangleMeshShape( _ref(btConvexTriangleMeshShape)* _this ) { free_ref(_this); }
static void finalize_btBoxShape( _ref(btBoxShape)* _this ) { free_ref(_this); }
static void finalize_btCapsuleShape( _ref(btCapsuleShape)* _this ) { free_ref(_this); }
static void finalize_btCapsuleShapeX( _ref(btCapsuleShapeX)* _this ) { free_ref(_this); }
static void finalize_btCapsuleShapeZ( _ref(btCapsuleShapeZ)* _this ) { free_ref(_this); }
static void finalize_btCylinderShape( _ref(btCylinderShape)* _this ) { free_ref(_this); }
static void finalize_btCylinderShapeX( _ref(btCylinderShapeX)* _this ) { free_ref(_this); }
static void finalize_btCylinderShapeZ( _ref(btCylinderShapeZ)* _this ) { free_ref(_this); }
static void finalize_btSphereShape( _ref(btSphereShape)* _this ) { free_ref(_this); }
static void finalize_btConeShape( _ref(btConeShape)* _this ) { free_ref(_this); }
static void finalize_btConvexHullShape( _ref(btConvexHullShape)* _this ) { free_ref(_this); }
static void finalize_btConeShapeX( _ref(btConeShapeX)* _this ) { free_ref(_this); }
static void finalize_btConeShapeZ( _ref(btConeShapeZ)* _this ) { free_ref(_this); }
static void finalize_btCompoundShape( _ref(btCompoundShape)* _this ) { free_ref(_this); }
static void finalize_btStridingMeshInterface( _ref(btStridingMeshInterface)* _this ) { free_ref(_this); }
static void finalize_btTriangleMesh( _ref(btTriangleMesh)* _this ) { free_ref(_this); }
static PHY_ScalarType PHY_ScalarType__values[] = { PHY_FLOAT,PHY_DOUBLE,PHY_INTEGER,PHY_SHORT,PHY_FIXEDPOINT88,PHY_UCHAR };
static void finalize_btConcaveShape( _ref(btConcaveShape)* _this ) { free_ref(_this); }
static void finalize_btStaticPlaneShape( _ref(btStaticPlaneShape)* _this ) { free_ref(_this); }
static void finalize_btTriangleMeshShape( _ref(btTriangleMeshShape)* _this ) { free_ref(_this); }
static void finalize_btBvhTriangleMeshShape( _ref(btBvhTriangleMeshShape)* _this ) { free_ref(_this); }
static void finalize_btHeightfieldTerrainShape( _ref(btHeightfieldTerrainShape)* _this ) { free_ref(_this); }
static void finalize_btDefaultCollisionConstructionInfo( _ref(btDefaultCollisionConstructionInfo)* _this ) { free_ref(_this); }
static void finalize_btDefaultCollisionConfiguration( _ref(btDefaultCollisionConfiguration)* _this ) { free_ref(_this); }
static void finalize_btPersistentManifold( _ref(btPersistentManifold)* _this ) { free_ref(_this); }
static void finalize_btDispatcher( _ref(btDispatcher)* _this ) { free_ref(_this); }
static void finalize_btCollisionDispatcher( _ref(btCollisionDispatcher)* _this ) { free_ref(_this); }
static void finalize_btOverlappingPairCallback( _ref(btOverlappingPairCallback)* _this ) { free_ref(_this); }
static void finalize_btOverlappingPairCache( _ref(btOverlappingPairCache)* _this ) { free_ref(_this); }
static void finalize_btAxisSweep3( _ref(btAxisSweep3)* _this ) { free_ref(_this); }
static void finalize_btBroadphaseInterface( _ref(btBroadphaseInterface)* _this ) { free_ref(_this); }
static void finalize_btCollisionConfiguration( _ref(btCollisionConfiguration)* _this ) { free_ref(_this); }
static void finalize_btDbvtBroadphase( _ref(btDbvtBroadphase)* _this ) { free_ref(_this); }
static void finalize_btRigidBodyConstructionInfo( _ref(btRigidBody::btRigidBodyConstructionInfo)* _this ) { free_ref(_this); }
static void finalize_btRigidBody( _ref(btRigidBody)* _this ) { free_ref(_this); }
static void finalize_btConstraintSetting( _ref(btConstraintSetting)* _this ) { free_ref(_this); }
static void finalize_btTypedConstraint( _ref(btTypedConstraint)* _this ) { free_ref(_this); }
static btConstraintParams btConstraintParams__values[] = { BT_CONSTRAINT_ERP,BT_CONSTRAINT_STOP_ERP,BT_CONSTRAINT_CFM,BT_CONSTRAINT_STOP_CFM };
static void finalize_btPoint2PointConstraint( _ref(btPoint2PointConstraint)* _this ) { free_ref(_this); }
static void finalize_btGeneric6DofConstraint( _ref(btGeneric6DofConstraint)* _this ) { free_ref(_this); }
static void finalize_btGeneric6DofSpringConstraint( _ref(btGeneric6DofSpringConstraint)* _this ) { free_ref(_this); }
static void finalize_btSequentialImpulseConstraintSolver( _ref(btSequentialImpulseConstraintSolver)* _this ) { free_ref(_this); }
static void finalize_btConeTwistConstraint( _ref(btConeTwistConstraint)* _this ) { free_ref(_this); }
static void finalize_btHingeConstraint( _ref(btHingeConstraint)* _this ) { free_ref(_this); }
static void finalize_btSliderConstraint( _ref(btSliderConstraint)* _this ) { free_ref(_this); }
static void finalize_btFixedConstraint( _ref(btFixedConstraint)* _this ) { free_ref(_this); }
static void finalize_btConstraintSolver( _ref(btConstraintSolver)* _this ) { free_ref(_this); }
static void finalize_btDispatcherInfo( _ref(btDispatcherInfo)* _this ) { free_ref(_this); }
static void finalize_btCollisionWorld( _ref(btCollisionWorld)* _this ) { free_ref(_this); }
static void finalize_btContactSolverInfo( _ref(btContactSolverInfo)* _this ) { free_ref(_this); }
static void finalize_btDynamicsWorld( _ref(btDynamicsWorld)* _this ) { free_ref(_this); }
static void finalize_btDiscreteDynamicsWorld( _ref(btDiscreteDynamicsWorld)* _this ) { free_ref(_this); }
static void finalize_btVehicleTuning( _ref(btRaycastVehicle::btVehicleTuning)* _this ) { free_ref(_this); }
static void finalize_btVehicleRaycasterResult( _ref(btDefaultVehicleRaycaster::btVehicleRaycasterResult)* _this ) { free_ref(_this); }
static void finalize_btVehicleRaycaster( _ref(btVehicleRaycaster)* _this ) { free_ref(_this); }
static void finalize_btDefaultVehicleRaycaster( _ref(btDefaultVehicleRaycaster)* _this ) { free_ref(_this); }
static void finalize_RaycastInfo( _ref(btWheelInfo::RaycastInfo)* _this ) { free_ref(_this); }
static void finalize_btWheelInfoConstructionInfo( _ref(btWheelInfoConstructionInfo)* _this ) { free_ref(_this); }
static void finalize_btWheelInfo( _ref(btWheelInfo)* _this ) { free_ref(_this); }
static void finalize_btActionInterface( _ref(btActionInterface)* _this ) { free_ref(_this); }
static void finalize_btKinematicCharacterController( _ref(btKinematicCharacterController)* _this ) { free_ref(_this); }
static void finalize_btRaycastVehicle( _ref(btRaycastVehicle)* _this ) { free_ref(_this); }
static void finalize_btGhostObject( _ref(btGhostObject)* _this ) { free_ref(_this); }
static void finalize_btPairCachingGhostObject( _ref(btPairCachingGhostObject)* _this ) { free_ref(_this); }
static void finalize_btGhostPairCallback( _ref(btGhostPairCallback)* _this ) { free_ref(_this); }
static void finalize_btSoftBodyWorldInfo( _ref(btSoftBodyWorldInfo)* _this ) { free_ref(_this); }
static void finalize_Node( _ref(btSoftBody::Node)* _this ) { free_ref(_this); }
static void finalize_tNodeArray( _ref(btSoftBody::tNodeArray)* _this ) { free_ref(_this); }
static void finalize_Material( _ref(btSoftBody::Material)* _this ) { free_ref(_this); }
static void finalize_tMaterialArray( _ref(btSoftBody::tMaterialArray)* _this ) { free_ref(_this); }
static void finalize_Config( _ref(btSoftBody::Config)* _this ) { free_ref(_this); }
static void finalize_btSoftBody( _ref(btSoftBody)* _this ) { free_ref(_this); }
static void finalize_btSoftBodyRigidBodyCollisionConfiguration( _ref(btSoftBodyRigidBodyCollisionConfiguration)* _this ) { free_ref(_this); }
static void finalize_btSoftBodySolver( _ref(btSoftBodySolver)* _this ) { free_ref(_this); }
static void finalize_btDefaultSoftBodySolver( _ref(btDefaultSoftBodySolver)* _this ) { free_ref(_this); }
static void finalize_btSoftBodyArray( _ref(btSoftBodyArray)* _this ) { free_ref(_this); }
static void finalize_btSoftRigidDynamicsWorld( _ref(btSoftRigidDynamicsWorld)* _this ) { free_ref(_this); }
static void finalize_btSoftBodyHelpers( _ref(btSoftBodyHelpers)* _this ) { free_ref(_this); }
HL_PRIM _ref(btVector3)* HL_NAME(btVector3_new0)() {
	return alloc_ref((new btVector3()),finalize_btVector3);
}
DEFINE_PRIM(_IDL, btVector3_new0,);

HL_PRIM _ref(btVector3)* HL_NAME(btVector3_new3)(float x, float y, float z) {
	return alloc_ref((new btVector3(x, y, z)),finalize_btVector3);
}
DEFINE_PRIM(_IDL, btVector3_new3, _F32 _F32 _F32);

HL_PRIM float HL_NAME(btVector3_length)(_ref(btVector3)* _this) {
	return _unref(_this)->length();
}
DEFINE_PRIM(_F32, btVector3_length, _IDL);

HL_PRIM float HL_NAME(btVector3_x)(_ref(btVector3)* _this) {
	return _unref(_this)->x();
}
DEFINE_PRIM(_F32, btVector3_x, _IDL);

HL_PRIM float HL_NAME(btVector3_y)(_ref(btVector3)* _this) {
	return _unref(_this)->y();
}
DEFINE_PRIM(_F32, btVector3_y, _IDL);

HL_PRIM float HL_NAME(btVector3_z)(_ref(btVector3)* _this) {
	return _unref(_this)->z();
}
DEFINE_PRIM(_F32, btVector3_z, _IDL);

HL_PRIM void HL_NAME(btVector3_setX)(_ref(btVector3)* _this, float x) {
	_unref(_this)->setX(x);
}
DEFINE_PRIM(_VOID, btVector3_setX, _IDL _F32);

HL_PRIM void HL_NAME(btVector3_setY)(_ref(btVector3)* _this, float y) {
	_unref(_this)->setY(y);
}
DEFINE_PRIM(_VOID, btVector3_setY, _IDL _F32);

HL_PRIM void HL_NAME(btVector3_setZ)(_ref(btVector3)* _this, float z) {
	_unref(_this)->setZ(z);
}
DEFINE_PRIM(_VOID, btVector3_setZ, _IDL _F32);

HL_PRIM void HL_NAME(btVector3_setValue)(_ref(btVector3)* _this, float x, float y, float z) {
	_unref(_this)->setValue(x, y, z);
}
DEFINE_PRIM(_VOID, btVector3_setValue, _IDL _F32 _F32 _F32);

HL_PRIM void HL_NAME(btVector3_normalize)(_ref(btVector3)* _this) {
	_unref(_this)->normalize();
}
DEFINE_PRIM(_VOID, btVector3_normalize, _IDL);

HL_PRIM _ref(btVector3)* HL_NAME(btVector3_rotate)(_ref(btVector3)* _this, _ref(btVector3)* wAxis, float angle) {
	return alloc_ref(new btVector3(_unref(_this)->rotate(*_unref(wAxis), angle)),finalize_btVector3);
}
DEFINE_PRIM(_IDL, btVector3_rotate, _IDL _IDL _F32);

HL_PRIM float HL_NAME(btVector3_dot)(_ref(btVector3)* _this, _ref(btVector3)* v) {
	return _unref(_this)->dot(*_unref(v));
}
DEFINE_PRIM(_F32, btVector3_dot, _IDL _IDL);

HL_PRIM _ref(btVector3)* HL_NAME(btVector3_op_mul)(_ref(btVector3)* _this, float x) {
	return alloc_ref(new btVector3(*_unref(_this) * (x)),finalize_btVector3);
}
DEFINE_PRIM(_IDL, btVector3_op_mul, _IDL _F32);

HL_PRIM _ref(btVector3)* HL_NAME(btVector3_op_add)(_ref(btVector3)* _this, _ref(btVector3)* v) {
	return alloc_ref(new btVector3(*_unref(_this) + (*_unref(v))),finalize_btVector3);
}
DEFINE_PRIM(_IDL, btVector3_op_add, _IDL _IDL);

HL_PRIM _ref(btVector3)* HL_NAME(btVector3_op_sub)(_ref(btVector3)* _this, _ref(btVector3)* v) {
	return alloc_ref(new btVector3(*_unref(_this) - (*_unref(v))),finalize_btVector3);
}
DEFINE_PRIM(_IDL, btVector3_op_sub, _IDL _IDL);

HL_PRIM _ref(btVector4)* HL_NAME(btVector4_new0)() {
	return alloc_ref((new btVector4()),finalize_btVector4);
}
DEFINE_PRIM(_IDL, btVector4_new0,);

HL_PRIM _ref(btVector4)* HL_NAME(btVector4_new4)(float x, float y, float z, float w) {
	return alloc_ref((new btVector4(x, y, z, w)),finalize_btVector4);
}
DEFINE_PRIM(_IDL, btVector4_new4, _F32 _F32 _F32 _F32);

HL_PRIM float HL_NAME(btVector4_w)(_ref(btVector4)* _this) {
	return _unref(_this)->w();
}
DEFINE_PRIM(_F32, btVector4_w, _IDL);

HL_PRIM void HL_NAME(btVector4_setValue)(_ref(btVector4)* _this, float x, float y, float z, float w) {
	_unref(_this)->setValue(x, y, z, w);
}
DEFINE_PRIM(_VOID, btVector4_setValue, _IDL _F32 _F32 _F32 _F32);

HL_PRIM float HL_NAME(btQuadWord_x)(_ref(btQuadWord)* _this) {
	return _unref(_this)->x();
}
DEFINE_PRIM(_F32, btQuadWord_x, _IDL);

HL_PRIM float HL_NAME(btQuadWord_y)(_ref(btQuadWord)* _this) {
	return _unref(_this)->y();
}
DEFINE_PRIM(_F32, btQuadWord_y, _IDL);

HL_PRIM float HL_NAME(btQuadWord_z)(_ref(btQuadWord)* _this) {
	return _unref(_this)->z();
}
DEFINE_PRIM(_F32, btQuadWord_z, _IDL);

HL_PRIM float HL_NAME(btQuadWord_w)(_ref(btQuadWord)* _this) {
	return _unref(_this)->w();
}
DEFINE_PRIM(_F32, btQuadWord_w, _IDL);

HL_PRIM void HL_NAME(btQuadWord_setX)(_ref(btQuadWord)* _this, float x) {
	_unref(_this)->setX(x);
}
DEFINE_PRIM(_VOID, btQuadWord_setX, _IDL _F32);

HL_PRIM void HL_NAME(btQuadWord_setY)(_ref(btQuadWord)* _this, float y) {
	_unref(_this)->setY(y);
}
DEFINE_PRIM(_VOID, btQuadWord_setY, _IDL _F32);

HL_PRIM void HL_NAME(btQuadWord_setZ)(_ref(btQuadWord)* _this, float z) {
	_unref(_this)->setZ(z);
}
DEFINE_PRIM(_VOID, btQuadWord_setZ, _IDL _F32);

HL_PRIM void HL_NAME(btQuadWord_setW)(_ref(btQuadWord)* _this, float w) {
	_unref(_this)->setW(w);
}
DEFINE_PRIM(_VOID, btQuadWord_setW, _IDL _F32);

HL_PRIM _ref(btQuaternion)* HL_NAME(btQuaternion_new4)(float x, float y, float z, float w) {
	return alloc_ref((new btQuaternion(x, y, z, w)),finalize_btQuaternion);
}
DEFINE_PRIM(_IDL, btQuaternion_new4, _F32 _F32 _F32 _F32);

HL_PRIM void HL_NAME(btQuaternion_setValue)(_ref(btQuaternion)* _this, float x, float y, float z, float w) {
	_unref(_this)->setValue(x, y, z, w);
}
DEFINE_PRIM(_VOID, btQuaternion_setValue, _IDL _F32 _F32 _F32 _F32);

HL_PRIM void HL_NAME(btQuaternion_setEulerZYX)(_ref(btQuaternion)* _this, float z, float y, float x) {
	_unref(_this)->setEulerZYX(z, y, x);
}
DEFINE_PRIM(_VOID, btQuaternion_setEulerZYX, _IDL _F32 _F32 _F32);

HL_PRIM void HL_NAME(btQuaternion_setRotation)(_ref(btQuaternion)* _this, _ref(btVector3)* axis, float angle) {
	_unref(_this)->setRotation(*_unref(axis), angle);
}
DEFINE_PRIM(_VOID, btQuaternion_setRotation, _IDL _IDL _F32);

HL_PRIM void HL_NAME(btQuaternion_normalize)(_ref(btQuaternion)* _this) {
	_unref(_this)->normalize();
}
DEFINE_PRIM(_VOID, btQuaternion_normalize, _IDL);

HL_PRIM float HL_NAME(btQuaternion_length2)(_ref(btQuaternion)* _this) {
	return _unref(_this)->length2();
}
DEFINE_PRIM(_F32, btQuaternion_length2, _IDL);

HL_PRIM float HL_NAME(btQuaternion_length)(_ref(btQuaternion)* _this) {
	return _unref(_this)->length();
}
DEFINE_PRIM(_F32, btQuaternion_length, _IDL);

HL_PRIM float HL_NAME(btQuaternion_dot)(_ref(btQuaternion)* _this, _ref(btQuaternion)* q) {
	return _unref(_this)->dot(*_unref(q));
}
DEFINE_PRIM(_F32, btQuaternion_dot, _IDL _IDL);

HL_PRIM _ref(btQuaternion)* HL_NAME(btQuaternion_normalized)(_ref(btQuaternion)* _this) {
	return alloc_ref(new btQuaternion(_unref(_this)->normalized()),finalize_btQuaternion);
}
DEFINE_PRIM(_IDL, btQuaternion_normalized, _IDL);

HL_PRIM _ref(btVector3)* HL_NAME(btQuaternion_getAxis)(_ref(btQuaternion)* _this) {
	return alloc_ref(new btVector3(_unref(_this)->getAxis()),finalize_btVector3);
}
DEFINE_PRIM(_IDL, btQuaternion_getAxis, _IDL);

HL_PRIM _ref(btQuaternion)* HL_NAME(btQuaternion_inverse)(_ref(btQuaternion)* _this) {
	return alloc_ref(new btQuaternion(_unref(_this)->inverse()),finalize_btQuaternion);
}
DEFINE_PRIM(_IDL, btQuaternion_inverse, _IDL);

HL_PRIM float HL_NAME(btQuaternion_getAngle)(_ref(btQuaternion)* _this) {
	return _unref(_this)->getAngle();
}
DEFINE_PRIM(_F32, btQuaternion_getAngle, _IDL);

HL_PRIM float HL_NAME(btQuaternion_getAngleShortestPath)(_ref(btQuaternion)* _this) {
	return _unref(_this)->getAngleShortestPath();
}
DEFINE_PRIM(_F32, btQuaternion_getAngleShortestPath, _IDL);

HL_PRIM float HL_NAME(btQuaternion_angle)(_ref(btQuaternion)* _this, _ref(btQuaternion)* q) {
	return _unref(_this)->angle(*_unref(q));
}
DEFINE_PRIM(_F32, btQuaternion_angle, _IDL _IDL);

HL_PRIM float HL_NAME(btQuaternion_angleShortestPath)(_ref(btQuaternion)* _this, _ref(btQuaternion)* q) {
	return _unref(_this)->angleShortestPath(*_unref(q));
}
DEFINE_PRIM(_F32, btQuaternion_angleShortestPath, _IDL _IDL);

HL_PRIM _ref(btQuaternion)* HL_NAME(btQuaternion_op_add)(_ref(btQuaternion)* _this, _ref(btQuaternion)* q) {
	return alloc_ref(new btQuaternion(*_unref(_this) + (*_unref(q))),finalize_btQuaternion);
}
DEFINE_PRIM(_IDL, btQuaternion_op_add, _IDL _IDL);

HL_PRIM _ref(btQuaternion)* HL_NAME(btQuaternion_op_sub)(_ref(btQuaternion)* _this, _ref(btQuaternion)* q) {
	return alloc_ref(new btQuaternion(*_unref(_this) - (*_unref(q))),finalize_btQuaternion);
}
DEFINE_PRIM(_IDL, btQuaternion_op_sub, _IDL _IDL);

HL_PRIM _ref(btQuaternion)* HL_NAME(btQuaternion_op_mul)(_ref(btQuaternion)* _this, float s) {
	return alloc_ref(new btQuaternion(*_unref(_this) * (s)),finalize_btQuaternion);
}
DEFINE_PRIM(_IDL, btQuaternion_op_mul, _IDL _F32);

HL_PRIM _ref(btQuaternion)* HL_NAME(btQuaternion_op_mulq)(_ref(btQuaternion)* _this, _ref(btQuaternion)* q) {
	return alloc_ref(new btQuaternion(*_unref(_this) *= (*_unref(q))),finalize_btQuaternion);
}
DEFINE_PRIM(_IDL, btQuaternion_op_mulq, _IDL _IDL);

HL_PRIM _ref(btQuaternion)* HL_NAME(btQuaternion_op_div)(_ref(btQuaternion)* _this, float s) {
	return alloc_ref(new btQuaternion(*_unref(_this) / (s)),finalize_btQuaternion);
}
DEFINE_PRIM(_IDL, btQuaternion_op_div, _IDL _F32);

HL_PRIM void HL_NAME(btMatrix3x3_setEulerZYX)(_ref(btMatrix3x3)* _this, float ex, float ey, float ez) {
	_unref(_this)->setEulerZYX(ex, ey, ez);
}
DEFINE_PRIM(_VOID, btMatrix3x3_setEulerZYX, _IDL _F32 _F32 _F32);

HL_PRIM void HL_NAME(btMatrix3x3_getRotation)(_ref(btMatrix3x3)* _this, _ref(btQuaternion)* q) {
	_unref(_this)->getRotation(*_unref(q));
}
DEFINE_PRIM(_VOID, btMatrix3x3_getRotation, _IDL _IDL);

HL_PRIM _ref(btVector3)* HL_NAME(btMatrix3x3_getRow)(_ref(btMatrix3x3)* _this, int y) {
	return alloc_ref(new btVector3(_unref(_this)->getRow(y)),finalize_btVector3);
}
DEFINE_PRIM(_IDL, btMatrix3x3_getRow, _IDL _I32);

HL_PRIM _ref(btTransform)* HL_NAME(btTransform_new0)() {
	return alloc_ref((new btTransform()),finalize_btTransform);
}
DEFINE_PRIM(_IDL, btTransform_new0,);

HL_PRIM _ref(btTransform)* HL_NAME(btTransform_new2)(_ref(btQuaternion)* q, _ref(btVector3)* v) {
	return alloc_ref((new btTransform(*_unref(q), *_unref(v))),finalize_btTransform);
}
DEFINE_PRIM(_IDL, btTransform_new2, _IDL _IDL);

HL_PRIM void HL_NAME(btTransform_setIdentity)(_ref(btTransform)* _this) {
	_unref(_this)->setIdentity();
}
DEFINE_PRIM(_VOID, btTransform_setIdentity, _IDL);

HL_PRIM void HL_NAME(btTransform_setOrigin)(_ref(btTransform)* _this, _ref(btVector3)* origin) {
	_unref(_this)->setOrigin(*_unref(origin));
}
DEFINE_PRIM(_VOID, btTransform_setOrigin, _IDL _IDL);

HL_PRIM void HL_NAME(btTransform_setRotation)(_ref(btTransform)* _this, _ref(btQuaternion)* rotation) {
	_unref(_this)->setRotation(*_unref(rotation));
}
DEFINE_PRIM(_VOID, btTransform_setRotation, _IDL _IDL);

HL_PRIM _ref(btVector3)* HL_NAME(btTransform_getOrigin)(_ref(btTransform)* _this) {
	return alloc_ref(new btVector3(_unref(_this)->getOrigin()),finalize_btVector3);
}
DEFINE_PRIM(_IDL, btTransform_getOrigin, _IDL);

HL_PRIM _ref(btQuaternion)* HL_NAME(btTransform_getRotation)(_ref(btTransform)* _this) {
	return alloc_ref(new btQuaternion(_unref(_this)->getRotation()),finalize_btQuaternion);
}
DEFINE_PRIM(_IDL, btTransform_getRotation, _IDL);

HL_PRIM _ref(btMatrix3x3)* HL_NAME(btTransform_getBasis)(_ref(btTransform)* _this) {
	return alloc_ref(new btMatrix3x3(_unref(_this)->getBasis()),finalize_btMatrix3x3);
}
DEFINE_PRIM(_IDL, btTransform_getBasis, _IDL);

HL_PRIM void HL_NAME(btTransform_setFromOpenGLMatrix)(_ref(btTransform)* _this, float* m) {
	_unref(_this)->setFromOpenGLMatrix(m);
}
DEFINE_PRIM(_VOID, btTransform_setFromOpenGLMatrix, _IDL _BYTES);

HL_PRIM void HL_NAME(btMotionState_getWorldTransform)(_ref(btMotionState)* _this, _ref(btTransform)* worldTrans) {
	_unref(_this)->getWorldTransform(*_unref(worldTrans));
}
DEFINE_PRIM(_VOID, btMotionState_getWorldTransform, _IDL _IDL);

HL_PRIM void HL_NAME(btMotionState_setWorldTransform)(_ref(btMotionState)* _this, _ref(btTransform)* worldTrans) {
	_unref(_this)->setWorldTransform(*_unref(worldTrans));
}
DEFINE_PRIM(_VOID, btMotionState_setWorldTransform, _IDL _IDL);

HL_PRIM _ref(btDefaultMotionState)* HL_NAME(btDefaultMotionState_new2)(_ref(btTransform)* startTrans, _ref(btTransform)* centerOfMassOffset) {
	if( !startTrans )
		return alloc_ref((new btDefaultMotionState()),finalize_btDefaultMotionState);
	else
	if( !centerOfMassOffset )
		return alloc_ref((new btDefaultMotionState(*_unref(startTrans))),finalize_btDefaultMotionState);
	else
		return alloc_ref((new btDefaultMotionState(*_unref(startTrans), *_unref(centerOfMassOffset))),finalize_btDefaultMotionState);
}
DEFINE_PRIM(_IDL, btDefaultMotionState_new2, _IDL _IDL);

HL_PRIM _ref(btTransform)* HL_NAME(btDefaultMotionState_get_m_graphicsWorldTrans)( _ref(btDefaultMotionState)* _this ) {
	return alloc_ref(new btTransform(_unref(_this)->m_graphicsWorldTrans),finalize_btTransform);
}
HL_PRIM void HL_NAME(btDefaultMotionState_set_m_graphicsWorldTrans)( _ref(btDefaultMotionState)* _this, _ref(btTransform)* value ) {
	_unref(_this)->m_graphicsWorldTrans = *_unref(value);
}

HL_PRIM void HL_NAME(btCollisionObject_setAnisotropicFriction)(_ref(btCollisionObject)* _this, _ref(btVector3)* anisotropicFriction, int frictionMode) {
	_unref(_this)->setAnisotropicFriction(*_unref(anisotropicFriction), frictionMode);
}
DEFINE_PRIM(_VOID, btCollisionObject_setAnisotropicFriction, _IDL _IDL _I32);

HL_PRIM _ref(btCollisionShape)* HL_NAME(btCollisionObject_getCollisionShape)(_ref(btCollisionObject)* _this) {
	return alloc_ref((_unref(_this)->getCollisionShape()),finalize_btCollisionShape);
}
DEFINE_PRIM(_IDL, btCollisionObject_getCollisionShape, _IDL);

HL_PRIM void HL_NAME(btCollisionObject_setContactProcessingThreshold)(_ref(btCollisionObject)* _this, float contactProcessingThreshold) {
	_unref(_this)->setContactProcessingThreshold(contactProcessingThreshold);
}
DEFINE_PRIM(_VOID, btCollisionObject_setContactProcessingThreshold, _IDL _F32);

HL_PRIM void HL_NAME(btCollisionObject_setActivationState)(_ref(btCollisionObject)* _this, int newState) {
	_unref(_this)->setActivationState(newState);
}
DEFINE_PRIM(_VOID, btCollisionObject_setActivationState, _IDL _I32);

HL_PRIM void HL_NAME(btCollisionObject_forceActivationState)(_ref(btCollisionObject)* _this, int newState) {
	_unref(_this)->forceActivationState(newState);
}
DEFINE_PRIM(_VOID, btCollisionObject_forceActivationState, _IDL _I32);

HL_PRIM void HL_NAME(btCollisionObject_activate)(_ref(btCollisionObject)* _this, vdynamic* forceActivation) {
	if( !forceActivation )
		_unref(_this)->activate();
	else
		_unref(_this)->activate(forceActivation->v.b);
}
DEFINE_PRIM(_VOID, btCollisionObject_activate, _IDL _NULL(_BOOL));

HL_PRIM bool HL_NAME(btCollisionObject_isActive)(_ref(btCollisionObject)* _this) {
	return _unref(_this)->isActive();
}
DEFINE_PRIM(_BOOL, btCollisionObject_isActive, _IDL);

HL_PRIM bool HL_NAME(btCollisionObject_isKinematicObject)(_ref(btCollisionObject)* _this) {
	return _unref(_this)->isKinematicObject();
}
DEFINE_PRIM(_BOOL, btCollisionObject_isKinematicObject, _IDL);

HL_PRIM bool HL_NAME(btCollisionObject_isStaticObject)(_ref(btCollisionObject)* _this) {
	return _unref(_this)->isStaticObject();
}
DEFINE_PRIM(_BOOL, btCollisionObject_isStaticObject, _IDL);

HL_PRIM bool HL_NAME(btCollisionObject_isStaticOrKinematicObject)(_ref(btCollisionObject)* _this) {
	return _unref(_this)->isStaticOrKinematicObject();
}
DEFINE_PRIM(_BOOL, btCollisionObject_isStaticOrKinematicObject, _IDL);

HL_PRIM void HL_NAME(btCollisionObject_setRestitution)(_ref(btCollisionObject)* _this, float rest) {
	_unref(_this)->setRestitution(rest);
}
DEFINE_PRIM(_VOID, btCollisionObject_setRestitution, _IDL _F32);

HL_PRIM void HL_NAME(btCollisionObject_setFriction)(_ref(btCollisionObject)* _this, float frict) {
	_unref(_this)->setFriction(frict);
}
DEFINE_PRIM(_VOID, btCollisionObject_setFriction, _IDL _F32);

HL_PRIM void HL_NAME(btCollisionObject_setRollingFriction)(_ref(btCollisionObject)* _this, float frict) {
	_unref(_this)->setRollingFriction(frict);
}
DEFINE_PRIM(_VOID, btCollisionObject_setRollingFriction, _IDL _F32);

HL_PRIM _ref(btTransform)* HL_NAME(btCollisionObject_getWorldTransform)(_ref(btCollisionObject)* _this) {
	return alloc_ref(new btTransform(_unref(_this)->getWorldTransform()),finalize_btTransform);
}
DEFINE_PRIM(_IDL, btCollisionObject_getWorldTransform, _IDL);

HL_PRIM int HL_NAME(btCollisionObject_getCollisionFlags)(_ref(btCollisionObject)* _this) {
	return _unref(_this)->getCollisionFlags();
}
DEFINE_PRIM(_I32, btCollisionObject_getCollisionFlags, _IDL);

HL_PRIM void HL_NAME(btCollisionObject_setCollisionFlags)(_ref(btCollisionObject)* _this, int flags) {
	_unref(_this)->setCollisionFlags(flags);
}
DEFINE_PRIM(_VOID, btCollisionObject_setCollisionFlags, _IDL _I32);

HL_PRIM void HL_NAME(btCollisionObject_setWorldTransform)(_ref(btCollisionObject)* _this, _ref(btTransform)* worldTrans) {
	_unref(_this)->setWorldTransform(*_unref(worldTrans));
}
DEFINE_PRIM(_VOID, btCollisionObject_setWorldTransform, _IDL _IDL);

HL_PRIM void HL_NAME(btCollisionObject_setCollisionShape)(_ref(btCollisionObject)* _this, _ref(btCollisionShape)* collisionShape) {
	_unref(_this)->setCollisionShape(_unref(collisionShape));
}
DEFINE_PRIM(_VOID, btCollisionObject_setCollisionShape, _IDL _IDL);

HL_PRIM void HL_NAME(btCollisionObject_setCcdMotionThreshold)(_ref(btCollisionObject)* _this, float ccdMotionThreshold) {
	_unref(_this)->setCcdMotionThreshold(ccdMotionThreshold);
}
DEFINE_PRIM(_VOID, btCollisionObject_setCcdMotionThreshold, _IDL _F32);

HL_PRIM void HL_NAME(btCollisionObject_setCcdSweptSphereRadius)(_ref(btCollisionObject)* _this, float radius) {
	_unref(_this)->setCcdSweptSphereRadius(radius);
}
DEFINE_PRIM(_VOID, btCollisionObject_setCcdSweptSphereRadius, _IDL _F32);

HL_PRIM int HL_NAME(btCollisionObject_getUserIndex)(_ref(btCollisionObject)* _this) {
	return _unref(_this)->getUserIndex();
}
DEFINE_PRIM(_I32, btCollisionObject_getUserIndex, _IDL);

HL_PRIM void HL_NAME(btCollisionObject_setUserIndex)(_ref(btCollisionObject)* _this, int index) {
	_unref(_this)->setUserIndex(index);
}
DEFINE_PRIM(_VOID, btCollisionObject_setUserIndex, _IDL _I32);

HL_PRIM void* HL_NAME(btCollisionObject_getUserPointer)(_ref(btCollisionObject)* _this) {
	return _unref(_this)->getUserPointer();
}
DEFINE_PRIM(_BYTES, btCollisionObject_getUserPointer, _IDL);

HL_PRIM void HL_NAME(btCollisionObject_setUserPointer)(_ref(btCollisionObject)* _this, void* userPointer) {
	_unref(_this)->setUserPointer(userPointer);
}
DEFINE_PRIM(_VOID, btCollisionObject_setUserPointer, _IDL _BYTES);

HL_PRIM bool HL_NAME(RayResultCallback_hasHit)(_ref(btCollisionWorld::RayResultCallback)* _this) {
	return _unref(_this)->hasHit();
}
DEFINE_PRIM(_BOOL, RayResultCallback_hasHit, _IDL);

HL_PRIM short HL_NAME(RayResultCallback_get_m_collisionFilterGroup)( _ref(btCollisionWorld::RayResultCallback)* _this ) {
	return _unref(_this)->m_collisionFilterGroup;
}
HL_PRIM void HL_NAME(RayResultCallback_set_m_collisionFilterGroup)( _ref(btCollisionWorld::RayResultCallback)* _this, short value ) {
	_unref(_this)->m_collisionFilterGroup = (value);
}

HL_PRIM short HL_NAME(RayResultCallback_get_m_collisionFilterMask)( _ref(btCollisionWorld::RayResultCallback)* _this ) {
	return _unref(_this)->m_collisionFilterMask;
}
HL_PRIM void HL_NAME(RayResultCallback_set_m_collisionFilterMask)( _ref(btCollisionWorld::RayResultCallback)* _this, short value ) {
	_unref(_this)->m_collisionFilterMask = (value);
}

HL_PRIM _ref(btCollisionObject)* HL_NAME(RayResultCallback_get_m_collisionObject)( _ref(btCollisionWorld::RayResultCallback)* _this ) {
	return alloc_ref_const(_unref(_this)->m_collisionObject,finalize_btCollisionObject);
}
HL_PRIM void HL_NAME(RayResultCallback_set_m_collisionObject)( _ref(btCollisionWorld::RayResultCallback)* _this, _ref(btCollisionObject)* value ) {
	_unref(_this)->m_collisionObject = _unref(value);
}

HL_PRIM _ref(btCollisionWorld::ClosestRayResultCallback)* HL_NAME(ClosestRayResultCallback_new2)(_ref(btVector3)* from, _ref(btVector3)* to) {
	return alloc_ref((new btCollisionWorld::ClosestRayResultCallback(*_unref(from), *_unref(to))),finalize_ClosestRayResultCallback);
}
DEFINE_PRIM(_IDL, ClosestRayResultCallback_new2, _IDL _IDL);

HL_PRIM _ref(btVector3)* HL_NAME(ClosestRayResultCallback_get_m_rayFromWorld)( _ref(btCollisionWorld::ClosestRayResultCallback)* _this ) {
	return alloc_ref(new btVector3(_unref(_this)->m_rayFromWorld),finalize_btVector3);
}
HL_PRIM void HL_NAME(ClosestRayResultCallback_set_m_rayFromWorld)( _ref(btCollisionWorld::ClosestRayResultCallback)* _this, _ref(btVector3)* value ) {
	_unref(_this)->m_rayFromWorld = *_unref(value);
}

HL_PRIM _ref(btVector3)* HL_NAME(ClosestRayResultCallback_get_m_rayToWorld)( _ref(btCollisionWorld::ClosestRayResultCallback)* _this ) {
	return alloc_ref(new btVector3(_unref(_this)->m_rayToWorld),finalize_btVector3);
}
HL_PRIM void HL_NAME(ClosestRayResultCallback_set_m_rayToWorld)( _ref(btCollisionWorld::ClosestRayResultCallback)* _this, _ref(btVector3)* value ) {
	_unref(_this)->m_rayToWorld = *_unref(value);
}

HL_PRIM _ref(btVector3)* HL_NAME(ClosestRayResultCallback_get_m_hitNormalWorld)( _ref(btCollisionWorld::ClosestRayResultCallback)* _this ) {
	return alloc_ref(new btVector3(_unref(_this)->m_hitNormalWorld),finalize_btVector3);
}
HL_PRIM void HL_NAME(ClosestRayResultCallback_set_m_hitNormalWorld)( _ref(btCollisionWorld::ClosestRayResultCallback)* _this, _ref(btVector3)* value ) {
	_unref(_this)->m_hitNormalWorld = *_unref(value);
}

HL_PRIM _ref(btVector3)* HL_NAME(ClosestRayResultCallback_get_m_hitPointWorld)( _ref(btCollisionWorld::ClosestRayResultCallback)* _this ) {
	return alloc_ref(new btVector3(_unref(_this)->m_hitPointWorld),finalize_btVector3);
}
HL_PRIM void HL_NAME(ClosestRayResultCallback_set_m_hitPointWorld)( _ref(btCollisionWorld::ClosestRayResultCallback)* _this, _ref(btVector3)* value ) {
	_unref(_this)->m_hitPointWorld = *_unref(value);
}

HL_PRIM _ref(btVector3)* HL_NAME(btManifoldPoint_getPositionWorldOnA)(_ref(btManifoldPoint)* _this) {
	return alloc_ref(new btVector3(_unref(_this)->getPositionWorldOnA()),finalize_btVector3);
}
DEFINE_PRIM(_IDL, btManifoldPoint_getPositionWorldOnA, _IDL);

HL_PRIM _ref(btVector3)* HL_NAME(btManifoldPoint_getPositionWorldOnB)(_ref(btManifoldPoint)* _this) {
	return alloc_ref(new btVector3(_unref(_this)->getPositionWorldOnB()),finalize_btVector3);
}
DEFINE_PRIM(_IDL, btManifoldPoint_getPositionWorldOnB, _IDL);

HL_PRIM double HL_NAME(btManifoldPoint_getAppliedImpulse)(_ref(btManifoldPoint)* _this) {
	return _unref(_this)->getAppliedImpulse();
}
DEFINE_PRIM(_F64, btManifoldPoint_getAppliedImpulse, _IDL);

HL_PRIM double HL_NAME(btManifoldPoint_getDistance)(_ref(btManifoldPoint)* _this) {
	return _unref(_this)->getDistance();
}
DEFINE_PRIM(_F64, btManifoldPoint_getDistance, _IDL);

HL_PRIM _ref(btVector3)* HL_NAME(btManifoldPoint_get_m_localPointA)( _ref(btManifoldPoint)* _this ) {
	return alloc_ref(new btVector3(_unref(_this)->m_localPointA),finalize_btVector3);
}
HL_PRIM void HL_NAME(btManifoldPoint_set_m_localPointA)( _ref(btManifoldPoint)* _this, _ref(btVector3)* value ) {
	_unref(_this)->m_localPointA = *_unref(value);
}

HL_PRIM _ref(btVector3)* HL_NAME(btManifoldPoint_get_m_localPointB)( _ref(btManifoldPoint)* _this ) {
	return alloc_ref(new btVector3(_unref(_this)->m_localPointB),finalize_btVector3);
}
HL_PRIM void HL_NAME(btManifoldPoint_set_m_localPointB)( _ref(btManifoldPoint)* _this, _ref(btVector3)* value ) {
	_unref(_this)->m_localPointB = *_unref(value);
}

HL_PRIM _ref(btVector3)* HL_NAME(btManifoldPoint_get_m_positionWorldOnB)( _ref(btManifoldPoint)* _this ) {
	return alloc_ref(new btVector3(_unref(_this)->m_positionWorldOnB),finalize_btVector3);
}
HL_PRIM void HL_NAME(btManifoldPoint_set_m_positionWorldOnB)( _ref(btManifoldPoint)* _this, _ref(btVector3)* value ) {
	_unref(_this)->m_positionWorldOnB = *_unref(value);
}

HL_PRIM _ref(btVector3)* HL_NAME(btManifoldPoint_get_m_positionWorldOnA)( _ref(btManifoldPoint)* _this ) {
	return alloc_ref(new btVector3(_unref(_this)->m_positionWorldOnA),finalize_btVector3);
}
HL_PRIM void HL_NAME(btManifoldPoint_set_m_positionWorldOnA)( _ref(btManifoldPoint)* _this, _ref(btVector3)* value ) {
	_unref(_this)->m_positionWorldOnA = *_unref(value);
}

HL_PRIM _ref(btVector3)* HL_NAME(btManifoldPoint_get_m_normalWorldOnB)( _ref(btManifoldPoint)* _this ) {
	return alloc_ref(new btVector3(_unref(_this)->m_normalWorldOnB),finalize_btVector3);
}
HL_PRIM void HL_NAME(btManifoldPoint_set_m_normalWorldOnB)( _ref(btManifoldPoint)* _this, _ref(btVector3)* value ) {
	_unref(_this)->m_normalWorldOnB = *_unref(value);
}

HL_PRIM float HL_NAME(ContactResultCallback_addSingleResult)(_ref(btCollisionWorld::ContactResultCallback)* _this, _ref(btManifoldPoint)* cp, _ref(btCollisionObjectWrapper)* colObj0Wrap, int partId0, int index0, _ref(btCollisionObjectWrapper)* colObj1Wrap, int partId1, int index1) {
	return _unref(_this)->addSingleResult(*_unref(cp), _unref(colObj0Wrap), partId0, index0, _unref(colObj1Wrap), partId1, index1);
}
DEFINE_PRIM(_F32, ContactResultCallback_addSingleResult, _IDL _IDL _IDL _I32 _I32 _IDL _I32 _I32);

HL_PRIM int HL_NAME(LocalShapeInfo_get_m_shapePart)( _ref(btCollisionWorld::LocalShapeInfo)* _this ) {
	return _unref(_this)->m_shapePart;
}
HL_PRIM void HL_NAME(LocalShapeInfo_set_m_shapePart)( _ref(btCollisionWorld::LocalShapeInfo)* _this, int value ) {
	_unref(_this)->m_shapePart = (value);
}

HL_PRIM int HL_NAME(LocalShapeInfo_get_m_triangleIndex)( _ref(btCollisionWorld::LocalShapeInfo)* _this ) {
	return _unref(_this)->m_triangleIndex;
}
HL_PRIM void HL_NAME(LocalShapeInfo_set_m_triangleIndex)( _ref(btCollisionWorld::LocalShapeInfo)* _this, int value ) {
	_unref(_this)->m_triangleIndex = (value);
}

HL_PRIM _ref(btCollisionWorld::LocalConvexResult)* HL_NAME(LocalConvexResult_new5)(_ref(btCollisionObject)* hitCollisionObject, _ref(btCollisionWorld::LocalShapeInfo)* localShapeInfo, _ref(btVector3)* hitNormalLocal, _ref(btVector3)* hitPointLocal, float hitFraction) {
	return alloc_ref((new btCollisionWorld::LocalConvexResult(_unref(hitCollisionObject), _unref(localShapeInfo), *_unref(hitNormalLocal), *_unref(hitPointLocal), hitFraction)),finalize_LocalConvexResult);
}
DEFINE_PRIM(_IDL, LocalConvexResult_new5, _IDL _IDL _IDL _IDL _F32);

HL_PRIM _ref(btCollisionObject)* HL_NAME(LocalConvexResult_get_m_hitCollisionObject)( _ref(btCollisionWorld::LocalConvexResult)* _this ) {
	return alloc_ref_const(_unref(_this)->m_hitCollisionObject,finalize_btCollisionObject);
}
HL_PRIM void HL_NAME(LocalConvexResult_set_m_hitCollisionObject)( _ref(btCollisionWorld::LocalConvexResult)* _this, _ref(btCollisionObject)* value ) {
	_unref(_this)->m_hitCollisionObject = _unref(value);
}

HL_PRIM _ref(btCollisionWorld::LocalShapeInfo)* HL_NAME(LocalConvexResult_get_m_localShapeInfo)( _ref(btCollisionWorld::LocalConvexResult)* _this ) {
	return alloc_ref(_unref(_this)->m_localShapeInfo,finalize_LocalShapeInfo);
}
HL_PRIM void HL_NAME(LocalConvexResult_set_m_localShapeInfo)( _ref(btCollisionWorld::LocalConvexResult)* _this, _ref(btCollisionWorld::LocalShapeInfo)* value ) {
	_unref(_this)->m_localShapeInfo = _unref(value);
}

HL_PRIM _ref(btVector3)* HL_NAME(LocalConvexResult_get_m_hitNormalLocal)( _ref(btCollisionWorld::LocalConvexResult)* _this ) {
	return alloc_ref(new btVector3(_unref(_this)->m_hitNormalLocal),finalize_btVector3);
}
HL_PRIM void HL_NAME(LocalConvexResult_set_m_hitNormalLocal)( _ref(btCollisionWorld::LocalConvexResult)* _this, _ref(btVector3)* value ) {
	_unref(_this)->m_hitNormalLocal = *_unref(value);
}

HL_PRIM _ref(btVector3)* HL_NAME(LocalConvexResult_get_m_hitPointLocal)( _ref(btCollisionWorld::LocalConvexResult)* _this ) {
	return alloc_ref(new btVector3(_unref(_this)->m_hitPointLocal),finalize_btVector3);
}
HL_PRIM void HL_NAME(LocalConvexResult_set_m_hitPointLocal)( _ref(btCollisionWorld::LocalConvexResult)* _this, _ref(btVector3)* value ) {
	_unref(_this)->m_hitPointLocal = *_unref(value);
}

HL_PRIM float HL_NAME(LocalConvexResult_get_m_hitFraction)( _ref(btCollisionWorld::LocalConvexResult)* _this ) {
	return _unref(_this)->m_hitFraction;
}
HL_PRIM void HL_NAME(LocalConvexResult_set_m_hitFraction)( _ref(btCollisionWorld::LocalConvexResult)* _this, float value ) {
	_unref(_this)->m_hitFraction = (value);
}

HL_PRIM bool HL_NAME(ConvexResultCallback_hasHit)(_ref(btCollisionWorld::ConvexResultCallback)* _this) {
	return _unref(_this)->hasHit();
}
DEFINE_PRIM(_BOOL, ConvexResultCallback_hasHit, _IDL);

HL_PRIM short HL_NAME(ConvexResultCallback_get_m_collisionFilterGroup)( _ref(btCollisionWorld::ConvexResultCallback)* _this ) {
	return _unref(_this)->m_collisionFilterGroup;
}
HL_PRIM void HL_NAME(ConvexResultCallback_set_m_collisionFilterGroup)( _ref(btCollisionWorld::ConvexResultCallback)* _this, short value ) {
	_unref(_this)->m_collisionFilterGroup = (value);
}

HL_PRIM short HL_NAME(ConvexResultCallback_get_m_collisionFilterMask)( _ref(btCollisionWorld::ConvexResultCallback)* _this ) {
	return _unref(_this)->m_collisionFilterMask;
}
HL_PRIM void HL_NAME(ConvexResultCallback_set_m_collisionFilterMask)( _ref(btCollisionWorld::ConvexResultCallback)* _this, short value ) {
	_unref(_this)->m_collisionFilterMask = (value);
}

HL_PRIM float HL_NAME(ConvexResultCallback_get_m_closestHitFraction)( _ref(btCollisionWorld::ConvexResultCallback)* _this ) {
	return _unref(_this)->m_closestHitFraction;
}
HL_PRIM void HL_NAME(ConvexResultCallback_set_m_closestHitFraction)( _ref(btCollisionWorld::ConvexResultCallback)* _this, float value ) {
	_unref(_this)->m_closestHitFraction = (value);
}

HL_PRIM _ref(btCollisionWorld::ClosestConvexResultCallback)* HL_NAME(ClosestConvexResultCallback_new2)(_ref(btVector3)* convexFromWorld, _ref(btVector3)* convexToWorld) {
	return alloc_ref((new btCollisionWorld::ClosestConvexResultCallback(*_unref(convexFromWorld), *_unref(convexToWorld))),finalize_ClosestConvexResultCallback);
}
DEFINE_PRIM(_IDL, ClosestConvexResultCallback_new2, _IDL _IDL);

HL_PRIM _ref(btVector3)* HL_NAME(ClosestConvexResultCallback_get_m_convexFromWorld)( _ref(btCollisionWorld::ClosestConvexResultCallback)* _this ) {
	return alloc_ref(new btVector3(_unref(_this)->m_convexFromWorld),finalize_btVector3);
}
HL_PRIM void HL_NAME(ClosestConvexResultCallback_set_m_convexFromWorld)( _ref(btCollisionWorld::ClosestConvexResultCallback)* _this, _ref(btVector3)* value ) {
	_unref(_this)->m_convexFromWorld = *_unref(value);
}

HL_PRIM _ref(btVector3)* HL_NAME(ClosestConvexResultCallback_get_m_convexToWorld)( _ref(btCollisionWorld::ClosestConvexResultCallback)* _this ) {
	return alloc_ref(new btVector3(_unref(_this)->m_convexToWorld),finalize_btVector3);
}
HL_PRIM void HL_NAME(ClosestConvexResultCallback_set_m_convexToWorld)( _ref(btCollisionWorld::ClosestConvexResultCallback)* _this, _ref(btVector3)* value ) {
	_unref(_this)->m_convexToWorld = *_unref(value);
}

HL_PRIM _ref(btVector3)* HL_NAME(ClosestConvexResultCallback_get_m_hitNormalWorld)( _ref(btCollisionWorld::ClosestConvexResultCallback)* _this ) {
	return alloc_ref(new btVector3(_unref(_this)->m_hitNormalWorld),finalize_btVector3);
}
HL_PRIM void HL_NAME(ClosestConvexResultCallback_set_m_hitNormalWorld)( _ref(btCollisionWorld::ClosestConvexResultCallback)* _this, _ref(btVector3)* value ) {
	_unref(_this)->m_hitNormalWorld = *_unref(value);
}

HL_PRIM _ref(btVector3)* HL_NAME(ClosestConvexResultCallback_get_m_hitPointWorld)( _ref(btCollisionWorld::ClosestConvexResultCallback)* _this ) {
	return alloc_ref(new btVector3(_unref(_this)->m_hitPointWorld),finalize_btVector3);
}
HL_PRIM void HL_NAME(ClosestConvexResultCallback_set_m_hitPointWorld)( _ref(btCollisionWorld::ClosestConvexResultCallback)* _this, _ref(btVector3)* value ) {
	_unref(_this)->m_hitPointWorld = *_unref(value);
}

HL_PRIM void HL_NAME(btCollisionShape_setLocalScaling)(_ref(btCollisionShape)* _this, _ref(btVector3)* scaling) {
	_unref(_this)->setLocalScaling(*_unref(scaling));
}
DEFINE_PRIM(_VOID, btCollisionShape_setLocalScaling, _IDL _IDL);

HL_PRIM void HL_NAME(btCollisionShape_calculateLocalInertia)(_ref(btCollisionShape)* _this, float mass, _ref(btVector3)* inertia) {
	_unref(_this)->calculateLocalInertia(mass, *_unref(inertia));
}
DEFINE_PRIM(_VOID, btCollisionShape_calculateLocalInertia, _IDL _F32 _IDL);

HL_PRIM void HL_NAME(btCollisionShape_setMargin)(_ref(btCollisionShape)* _this, float margin) {
	_unref(_this)->setMargin(margin);
}
DEFINE_PRIM(_VOID, btCollisionShape_setMargin, _IDL _F32);

HL_PRIM float HL_NAME(btCollisionShape_getMargin)(_ref(btCollisionShape)* _this) {
	return _unref(_this)->getMargin();
}
DEFINE_PRIM(_F32, btCollisionShape_getMargin, _IDL);

HL_PRIM _ref(btConvexTriangleMeshShape)* HL_NAME(btConvexTriangleMeshShape_new2)(_ref(btStridingMeshInterface)* meshInterface, vdynamic* calcAabb) {
	if( !calcAabb )
		return alloc_ref((new btConvexTriangleMeshShape(_unref(meshInterface))),finalize_btConvexTriangleMeshShape);
	else
		return alloc_ref((new btConvexTriangleMeshShape(_unref(meshInterface), calcAabb->v.b)),finalize_btConvexTriangleMeshShape);
}
DEFINE_PRIM(_IDL, btConvexTriangleMeshShape_new2, _IDL _NULL(_BOOL));

HL_PRIM _ref(btBoxShape)* HL_NAME(btBoxShape_new1)(_ref(btVector3)* boxHalfExtents) {
	return alloc_ref((new btBoxShape(*_unref(boxHalfExtents))),finalize_btBoxShape);
}
DEFINE_PRIM(_IDL, btBoxShape_new1, _IDL);

HL_PRIM void HL_NAME(btBoxShape_setMargin)(_ref(btBoxShape)* _this, float margin) {
	_unref(_this)->setMargin(margin);
}
DEFINE_PRIM(_VOID, btBoxShape_setMargin, _IDL _F32);

HL_PRIM float HL_NAME(btBoxShape_getMargin)(_ref(btBoxShape)* _this) {
	return _unref(_this)->getMargin();
}
DEFINE_PRIM(_F32, btBoxShape_getMargin, _IDL);

HL_PRIM _ref(btCapsuleShape)* HL_NAME(btCapsuleShape_new2)(float radius, float height) {
	return alloc_ref((new btCapsuleShape(radius, height)),finalize_btCapsuleShape);
}
DEFINE_PRIM(_IDL, btCapsuleShape_new2, _F32 _F32);

HL_PRIM void HL_NAME(btCapsuleShape_setMargin)(_ref(btCapsuleShape)* _this, float margin) {
	_unref(_this)->setMargin(margin);
}
DEFINE_PRIM(_VOID, btCapsuleShape_setMargin, _IDL _F32);

HL_PRIM float HL_NAME(btCapsuleShape_getMargin)(_ref(btCapsuleShape)* _this) {
	return _unref(_this)->getMargin();
}
DEFINE_PRIM(_F32, btCapsuleShape_getMargin, _IDL);

HL_PRIM _ref(btCapsuleShapeX)* HL_NAME(btCapsuleShapeX_new2)(float radius, float height) {
	return alloc_ref((new btCapsuleShapeX(radius, height)),finalize_btCapsuleShapeX);
}
DEFINE_PRIM(_IDL, btCapsuleShapeX_new2, _F32 _F32);

HL_PRIM void HL_NAME(btCapsuleShapeX_setMargin)(_ref(btCapsuleShapeX)* _this, float margin) {
	_unref(_this)->setMargin(margin);
}
DEFINE_PRIM(_VOID, btCapsuleShapeX_setMargin, _IDL _F32);

HL_PRIM float HL_NAME(btCapsuleShapeX_getMargin)(_ref(btCapsuleShapeX)* _this) {
	return _unref(_this)->getMargin();
}
DEFINE_PRIM(_F32, btCapsuleShapeX_getMargin, _IDL);

HL_PRIM _ref(btCapsuleShapeZ)* HL_NAME(btCapsuleShapeZ_new2)(float radius, float height) {
	return alloc_ref((new btCapsuleShapeZ(radius, height)),finalize_btCapsuleShapeZ);
}
DEFINE_PRIM(_IDL, btCapsuleShapeZ_new2, _F32 _F32);

HL_PRIM void HL_NAME(btCapsuleShapeZ_setMargin)(_ref(btCapsuleShapeZ)* _this, float margin) {
	_unref(_this)->setMargin(margin);
}
DEFINE_PRIM(_VOID, btCapsuleShapeZ_setMargin, _IDL _F32);

HL_PRIM float HL_NAME(btCapsuleShapeZ_getMargin)(_ref(btCapsuleShapeZ)* _this) {
	return _unref(_this)->getMargin();
}
DEFINE_PRIM(_F32, btCapsuleShapeZ_getMargin, _IDL);

HL_PRIM _ref(btCylinderShape)* HL_NAME(btCylinderShape_new1)(_ref(btVector3)* halfExtents) {
	return alloc_ref((new btCylinderShape(*_unref(halfExtents))),finalize_btCylinderShape);
}
DEFINE_PRIM(_IDL, btCylinderShape_new1, _IDL);

HL_PRIM void HL_NAME(btCylinderShape_setMargin)(_ref(btCylinderShape)* _this, float margin) {
	_unref(_this)->setMargin(margin);
}
DEFINE_PRIM(_VOID, btCylinderShape_setMargin, _IDL _F32);

HL_PRIM float HL_NAME(btCylinderShape_getMargin)(_ref(btCylinderShape)* _this) {
	return _unref(_this)->getMargin();
}
DEFINE_PRIM(_F32, btCylinderShape_getMargin, _IDL);

HL_PRIM _ref(btCylinderShapeX)* HL_NAME(btCylinderShapeX_new1)(_ref(btVector3)* halfExtents) {
	return alloc_ref((new btCylinderShapeX(*_unref(halfExtents))),finalize_btCylinderShapeX);
}
DEFINE_PRIM(_IDL, btCylinderShapeX_new1, _IDL);

HL_PRIM void HL_NAME(btCylinderShapeX_setMargin)(_ref(btCylinderShapeX)* _this, float margin) {
	_unref(_this)->setMargin(margin);
}
DEFINE_PRIM(_VOID, btCylinderShapeX_setMargin, _IDL _F32);

HL_PRIM float HL_NAME(btCylinderShapeX_getMargin)(_ref(btCylinderShapeX)* _this) {
	return _unref(_this)->getMargin();
}
DEFINE_PRIM(_F32, btCylinderShapeX_getMargin, _IDL);

HL_PRIM _ref(btCylinderShapeZ)* HL_NAME(btCylinderShapeZ_new1)(_ref(btVector3)* halfExtents) {
	return alloc_ref((new btCylinderShapeZ(*_unref(halfExtents))),finalize_btCylinderShapeZ);
}
DEFINE_PRIM(_IDL, btCylinderShapeZ_new1, _IDL);

HL_PRIM void HL_NAME(btCylinderShapeZ_setMargin)(_ref(btCylinderShapeZ)* _this, float margin) {
	_unref(_this)->setMargin(margin);
}
DEFINE_PRIM(_VOID, btCylinderShapeZ_setMargin, _IDL _F32);

HL_PRIM float HL_NAME(btCylinderShapeZ_getMargin)(_ref(btCylinderShapeZ)* _this) {
	return _unref(_this)->getMargin();
}
DEFINE_PRIM(_F32, btCylinderShapeZ_getMargin, _IDL);

HL_PRIM _ref(btSphereShape)* HL_NAME(btSphereShape_new1)(float radius) {
	return alloc_ref((new btSphereShape(radius)),finalize_btSphereShape);
}
DEFINE_PRIM(_IDL, btSphereShape_new1, _F32);

HL_PRIM void HL_NAME(btSphereShape_setMargin)(_ref(btSphereShape)* _this, float margin) {
	_unref(_this)->setMargin(margin);
}
DEFINE_PRIM(_VOID, btSphereShape_setMargin, _IDL _F32);

HL_PRIM float HL_NAME(btSphereShape_getMargin)(_ref(btSphereShape)* _this) {
	return _unref(_this)->getMargin();
}
DEFINE_PRIM(_F32, btSphereShape_getMargin, _IDL);

HL_PRIM _ref(btConeShape)* HL_NAME(btConeShape_new2)(float radius, float height) {
	return alloc_ref((new btConeShape(radius, height)),finalize_btConeShape);
}
DEFINE_PRIM(_IDL, btConeShape_new2, _F32 _F32);

HL_PRIM _ref(btConvexHullShape)* HL_NAME(btConvexHullShape_new0)() {
	return alloc_ref((new btConvexHullShape()),finalize_btConvexHullShape);
}
DEFINE_PRIM(_IDL, btConvexHullShape_new0,);

HL_PRIM void HL_NAME(btConvexHullShape_addPoint)(_ref(btConvexHullShape)* _this, _ref(btVector3)* point, vdynamic* recalculateLocalAABB) {
	if( !recalculateLocalAABB )
		_unref(_this)->addPoint(*_unref(point));
	else
		_unref(_this)->addPoint(*_unref(point), recalculateLocalAABB->v.b);
}
DEFINE_PRIM(_VOID, btConvexHullShape_addPoint, _IDL _IDL _NULL(_BOOL));

HL_PRIM void HL_NAME(btConvexHullShape_setMargin)(_ref(btConvexHullShape)* _this, float margin) {
	_unref(_this)->setMargin(margin);
}
DEFINE_PRIM(_VOID, btConvexHullShape_setMargin, _IDL _F32);

HL_PRIM float HL_NAME(btConvexHullShape_getMargin)(_ref(btConvexHullShape)* _this) {
	return _unref(_this)->getMargin();
}
DEFINE_PRIM(_F32, btConvexHullShape_getMargin, _IDL);

HL_PRIM _ref(btConeShapeX)* HL_NAME(btConeShapeX_new2)(float radius, float height) {
	return alloc_ref((new btConeShapeX(radius, height)),finalize_btConeShapeX);
}
DEFINE_PRIM(_IDL, btConeShapeX_new2, _F32 _F32);

HL_PRIM _ref(btConeShapeZ)* HL_NAME(btConeShapeZ_new2)(float radius, float height) {
	return alloc_ref((new btConeShapeZ(radius, height)),finalize_btConeShapeZ);
}
DEFINE_PRIM(_IDL, btConeShapeZ_new2, _F32 _F32);

HL_PRIM _ref(btCompoundShape)* HL_NAME(btCompoundShape_new1)(vdynamic* enableDynamicAabbTree) {
	if( !enableDynamicAabbTree )
		return alloc_ref((new btCompoundShape()),finalize_btCompoundShape);
	else
		return alloc_ref((new btCompoundShape(enableDynamicAabbTree->v.b)),finalize_btCompoundShape);
}
DEFINE_PRIM(_IDL, btCompoundShape_new1, _NULL(_BOOL));

HL_PRIM void HL_NAME(btCompoundShape_addChildShape)(_ref(btCompoundShape)* _this, _ref(btTransform)* localTransform, _ref(btCollisionShape)* shape) {
	_unref(_this)->addChildShape(*_unref(localTransform), _unref(shape));
}
DEFINE_PRIM(_VOID, btCompoundShape_addChildShape, _IDL _IDL _IDL);

HL_PRIM void HL_NAME(btCompoundShape_removeChildShapeByIndex)(_ref(btCompoundShape)* _this, int childShapeindex) {
	_unref(_this)->removeChildShapeByIndex(childShapeindex);
}
DEFINE_PRIM(_VOID, btCompoundShape_removeChildShapeByIndex, _IDL _I32);

HL_PRIM int HL_NAME(btCompoundShape_getNumChildShapes)(_ref(btCompoundShape)* _this) {
	return _unref(_this)->getNumChildShapes();
}
DEFINE_PRIM(_I32, btCompoundShape_getNumChildShapes, _IDL);

HL_PRIM _ref(btCollisionShape)* HL_NAME(btCompoundShape_getChildShape)(_ref(btCompoundShape)* _this, int index) {
	return alloc_ref((_unref(_this)->getChildShape(index)),finalize_btCollisionShape);
}
DEFINE_PRIM(_IDL, btCompoundShape_getChildShape, _IDL _I32);

HL_PRIM void HL_NAME(btCompoundShape_setMargin)(_ref(btCompoundShape)* _this, float margin) {
	_unref(_this)->setMargin(margin);
}
DEFINE_PRIM(_VOID, btCompoundShape_setMargin, _IDL _F32);

HL_PRIM float HL_NAME(btCompoundShape_getMargin)(_ref(btCompoundShape)* _this) {
	return _unref(_this)->getMargin();
}
DEFINE_PRIM(_F32, btCompoundShape_getMargin, _IDL);

HL_PRIM _ref(btTriangleMesh)* HL_NAME(btTriangleMesh_new2)(vdynamic* use32bitIndices, vdynamic* use4componentVertices) {
	if( !use32bitIndices )
		return alloc_ref((new btTriangleMesh()),finalize_btTriangleMesh);
	else
	if( !use4componentVertices )
		return alloc_ref((new btTriangleMesh(use32bitIndices->v.b)),finalize_btTriangleMesh);
	else
		return alloc_ref((new btTriangleMesh(use32bitIndices->v.b, use4componentVertices->v.b)),finalize_btTriangleMesh);
}
DEFINE_PRIM(_IDL, btTriangleMesh_new2, _NULL(_BOOL) _NULL(_BOOL));

HL_PRIM void HL_NAME(btTriangleMesh_addTriangle)(_ref(btTriangleMesh)* _this, _ref(btVector3)* vertex0, _ref(btVector3)* vertex1, _ref(btVector3)* vertex2, vdynamic* removeDuplicateVertices) {
	if( !removeDuplicateVertices )
		_unref(_this)->addTriangle(*_unref(vertex0), *_unref(vertex1), *_unref(vertex2));
	else
		_unref(_this)->addTriangle(*_unref(vertex0), *_unref(vertex1), *_unref(vertex2), removeDuplicateVertices->v.b);
}
DEFINE_PRIM(_VOID, btTriangleMesh_addTriangle, _IDL _IDL _IDL _IDL _NULL(_BOOL));

HL_PRIM _ref(btStaticPlaneShape)* HL_NAME(btStaticPlaneShape_new2)(_ref(btVector3)* planeNormal, float planeConstant) {
	return alloc_ref((new btStaticPlaneShape(*_unref(planeNormal), planeConstant)),finalize_btStaticPlaneShape);
}
DEFINE_PRIM(_IDL, btStaticPlaneShape_new2, _IDL _F32);

HL_PRIM _ref(btBvhTriangleMeshShape)* HL_NAME(btBvhTriangleMeshShape_new3)(_ref(btStridingMeshInterface)* meshInterface, bool useQuantizedAabbCompression, vdynamic* buildBvh) {
	if( !buildBvh )
		return alloc_ref((new btBvhTriangleMeshShape(_unref(meshInterface), useQuantizedAabbCompression)),finalize_btBvhTriangleMeshShape);
	else
		return alloc_ref((new btBvhTriangleMeshShape(_unref(meshInterface), useQuantizedAabbCompression, buildBvh->v.b)),finalize_btBvhTriangleMeshShape);
}
DEFINE_PRIM(_IDL, btBvhTriangleMeshShape_new3, _IDL _BOOL _NULL(_BOOL));

HL_PRIM _ref(btHeightfieldTerrainShape)* HL_NAME(btHeightfieldTerrainShape_new9)(int heightStickWidth, int heightStickLength, void* heightfieldData, float heightScale, float minHeight, float maxHeight, int upAxis, int hdt, bool flipQuadEdges) {
	return alloc_ref((new btHeightfieldTerrainShape(heightStickWidth, heightStickLength, heightfieldData, heightScale, minHeight, maxHeight, upAxis, PHY_ScalarType__values[hdt], flipQuadEdges)),finalize_btHeightfieldTerrainShape);
}
DEFINE_PRIM(_IDL, btHeightfieldTerrainShape_new9, _I32 _I32 _BYTES _F32 _F32 _F32 _I32 _I32 _BOOL);

HL_PRIM void HL_NAME(btHeightfieldTerrainShape_setMargin)(_ref(btHeightfieldTerrainShape)* _this, float margin) {
	_unref(_this)->setMargin(margin);
}
DEFINE_PRIM(_VOID, btHeightfieldTerrainShape_setMargin, _IDL _F32);

HL_PRIM float HL_NAME(btHeightfieldTerrainShape_getMargin)(_ref(btHeightfieldTerrainShape)* _this) {
	return _unref(_this)->getMargin();
}
DEFINE_PRIM(_F32, btHeightfieldTerrainShape_getMargin, _IDL);

HL_PRIM _ref(btDefaultCollisionConstructionInfo)* HL_NAME(btDefaultCollisionConstructionInfo_new0)() {
	return alloc_ref((new btDefaultCollisionConstructionInfo()),finalize_btDefaultCollisionConstructionInfo);
}
DEFINE_PRIM(_IDL, btDefaultCollisionConstructionInfo_new0,);

HL_PRIM _ref(btDefaultCollisionConfiguration)* HL_NAME(btDefaultCollisionConfiguration_new1)(_ref(btDefaultCollisionConstructionInfo)* info) {
	if( !info )
		return alloc_ref((new btDefaultCollisionConfiguration()),finalize_btDefaultCollisionConfiguration);
	else
		return alloc_ref((new btDefaultCollisionConfiguration(*_unref(info))),finalize_btDefaultCollisionConfiguration);
}
DEFINE_PRIM(_IDL, btDefaultCollisionConfiguration_new1, _IDL);

HL_PRIM _ref(btPersistentManifold)* HL_NAME(btPersistentManifold_new0)() {
	return alloc_ref((new btPersistentManifold()),finalize_btPersistentManifold);
}
DEFINE_PRIM(_IDL, btPersistentManifold_new0,);

HL_PRIM _ref(btCollisionObject)* HL_NAME(btPersistentManifold_getBody0)(_ref(btPersistentManifold)* _this) {
	return alloc_ref_const((_unref(_this)->getBody0()),finalize_btCollisionObject);
}
DEFINE_PRIM(_IDL, btPersistentManifold_getBody0, _IDL);

HL_PRIM _ref(btCollisionObject)* HL_NAME(btPersistentManifold_getBody1)(_ref(btPersistentManifold)* _this) {
	return alloc_ref_const((_unref(_this)->getBody1()),finalize_btCollisionObject);
}
DEFINE_PRIM(_IDL, btPersistentManifold_getBody1, _IDL);

HL_PRIM int HL_NAME(btPersistentManifold_getNumContacts)(_ref(btPersistentManifold)* _this) {
	return _unref(_this)->getNumContacts();
}
DEFINE_PRIM(_I32, btPersistentManifold_getNumContacts, _IDL);

HL_PRIM _ref(btManifoldPoint)* HL_NAME(btPersistentManifold_getContactPoint)(_ref(btPersistentManifold)* _this, int index) {
	return alloc_ref(new btManifoldPoint(_unref(_this)->getContactPoint(index)),finalize_btManifoldPoint);
}
DEFINE_PRIM(_IDL, btPersistentManifold_getContactPoint, _IDL _I32);

HL_PRIM int HL_NAME(btDispatcher_getNumManifolds)(_ref(btDispatcher)* _this) {
	return _unref(_this)->getNumManifolds();
}
DEFINE_PRIM(_I32, btDispatcher_getNumManifolds, _IDL);

HL_PRIM _ref(btPersistentManifold)* HL_NAME(btDispatcher_getManifoldByIndexInternal)(_ref(btDispatcher)* _this, int index) {
	return alloc_ref((_unref(_this)->getManifoldByIndexInternal(index)),finalize_btPersistentManifold);
}
DEFINE_PRIM(_IDL, btDispatcher_getManifoldByIndexInternal, _IDL _I32);

HL_PRIM _ref(btCollisionDispatcher)* HL_NAME(btCollisionDispatcher_new1)(_ref(btDefaultCollisionConfiguration)* conf) {
	return alloc_ref((new btCollisionDispatcher(_unref(conf))),finalize_btCollisionDispatcher);
}
DEFINE_PRIM(_IDL, btCollisionDispatcher_new1, _IDL);

HL_PRIM void HL_NAME(btOverlappingPairCache_setInternalGhostPairCallback)(_ref(btOverlappingPairCache)* _this, _ref(btOverlappingPairCallback)* ghostPairCallback) {
	_unref(_this)->setInternalGhostPairCallback(_unref(ghostPairCallback));
}
DEFINE_PRIM(_VOID, btOverlappingPairCache_setInternalGhostPairCallback, _IDL _IDL);

HL_PRIM _ref(btAxisSweep3)* HL_NAME(btAxisSweep3_new5)(_ref(btVector3)* worldAabbMin, _ref(btVector3)* worldAabbMax, vdynamic* maxHandles, _ref(btOverlappingPairCache)* pairCache, vdynamic* disableRaycastAccelerator) {
	if( !maxHandles )
		return alloc_ref((new btAxisSweep3(*_unref(worldAabbMin), *_unref(worldAabbMax))),finalize_btAxisSweep3);
	else
	if( !pairCache )
		return alloc_ref((new btAxisSweep3(*_unref(worldAabbMin), *_unref(worldAabbMax), maxHandles->v.i)),finalize_btAxisSweep3);
	else
	if( !disableRaycastAccelerator )
		return alloc_ref((new btAxisSweep3(*_unref(worldAabbMin), *_unref(worldAabbMax), maxHandles->v.i, _unref(pairCache))),finalize_btAxisSweep3);
	else
		return alloc_ref((new btAxisSweep3(*_unref(worldAabbMin), *_unref(worldAabbMax), maxHandles->v.i, _unref(pairCache), disableRaycastAccelerator->v.b)),finalize_btAxisSweep3);
}
DEFINE_PRIM(_IDL, btAxisSweep3_new5, _IDL _IDL _NULL(_I32) _IDL _NULL(_BOOL));

HL_PRIM _ref(btDbvtBroadphase)* HL_NAME(btDbvtBroadphase_new0)() {
	return alloc_ref((new btDbvtBroadphase()),finalize_btDbvtBroadphase);
}
DEFINE_PRIM(_IDL, btDbvtBroadphase_new0,);

HL_PRIM _ref(btRigidBody::btRigidBodyConstructionInfo)* HL_NAME(btRigidBodyConstructionInfo_new4)(float mass, _ref(btMotionState)* motionState, _ref(btCollisionShape)* collisionShape, _ref(btVector3)* localInertia) {
	if( !localInertia )
		return alloc_ref((new btRigidBody::btRigidBodyConstructionInfo(mass, _unref(motionState), _unref(collisionShape))),finalize_btRigidBodyConstructionInfo);
	else
		return alloc_ref((new btRigidBody::btRigidBodyConstructionInfo(mass, _unref(motionState), _unref(collisionShape), *_unref(localInertia))),finalize_btRigidBodyConstructionInfo);
}
DEFINE_PRIM(_IDL, btRigidBodyConstructionInfo_new4, _F32 _IDL _IDL _IDL);

HL_PRIM float HL_NAME(btRigidBodyConstructionInfo_get_m_linearDamping)( _ref(btRigidBody::btRigidBodyConstructionInfo)* _this ) {
	return _unref(_this)->m_linearDamping;
}
HL_PRIM void HL_NAME(btRigidBodyConstructionInfo_set_m_linearDamping)( _ref(btRigidBody::btRigidBodyConstructionInfo)* _this, float value ) {
	_unref(_this)->m_linearDamping = (value);
}

HL_PRIM float HL_NAME(btRigidBodyConstructionInfo_get_m_angularDamping)( _ref(btRigidBody::btRigidBodyConstructionInfo)* _this ) {
	return _unref(_this)->m_angularDamping;
}
HL_PRIM void HL_NAME(btRigidBodyConstructionInfo_set_m_angularDamping)( _ref(btRigidBody::btRigidBodyConstructionInfo)* _this, float value ) {
	_unref(_this)->m_angularDamping = (value);
}

HL_PRIM float HL_NAME(btRigidBodyConstructionInfo_get_m_friction)( _ref(btRigidBody::btRigidBodyConstructionInfo)* _this ) {
	return _unref(_this)->m_friction;
}
HL_PRIM void HL_NAME(btRigidBodyConstructionInfo_set_m_friction)( _ref(btRigidBody::btRigidBodyConstructionInfo)* _this, float value ) {
	_unref(_this)->m_friction = (value);
}

HL_PRIM float HL_NAME(btRigidBodyConstructionInfo_get_m_rollingFriction)( _ref(btRigidBody::btRigidBodyConstructionInfo)* _this ) {
	return _unref(_this)->m_rollingFriction;
}
HL_PRIM void HL_NAME(btRigidBodyConstructionInfo_set_m_rollingFriction)( _ref(btRigidBody::btRigidBodyConstructionInfo)* _this, float value ) {
	_unref(_this)->m_rollingFriction = (value);
}

HL_PRIM float HL_NAME(btRigidBodyConstructionInfo_get_m_restitution)( _ref(btRigidBody::btRigidBodyConstructionInfo)* _this ) {
	return _unref(_this)->m_restitution;
}
HL_PRIM void HL_NAME(btRigidBodyConstructionInfo_set_m_restitution)( _ref(btRigidBody::btRigidBodyConstructionInfo)* _this, float value ) {
	_unref(_this)->m_restitution = (value);
}

HL_PRIM float HL_NAME(btRigidBodyConstructionInfo_get_m_linearSleepingThreshold)( _ref(btRigidBody::btRigidBodyConstructionInfo)* _this ) {
	return _unref(_this)->m_linearSleepingThreshold;
}
HL_PRIM void HL_NAME(btRigidBodyConstructionInfo_set_m_linearSleepingThreshold)( _ref(btRigidBody::btRigidBodyConstructionInfo)* _this, float value ) {
	_unref(_this)->m_linearSleepingThreshold = (value);
}

HL_PRIM float HL_NAME(btRigidBodyConstructionInfo_get_m_angularSleepingThreshold)( _ref(btRigidBody::btRigidBodyConstructionInfo)* _this ) {
	return _unref(_this)->m_angularSleepingThreshold;
}
HL_PRIM void HL_NAME(btRigidBodyConstructionInfo_set_m_angularSleepingThreshold)( _ref(btRigidBody::btRigidBodyConstructionInfo)* _this, float value ) {
	_unref(_this)->m_angularSleepingThreshold = (value);
}

HL_PRIM bool HL_NAME(btRigidBodyConstructionInfo_get_m_additionalDamping)( _ref(btRigidBody::btRigidBodyConstructionInfo)* _this ) {
	return _unref(_this)->m_additionalDamping;
}
HL_PRIM void HL_NAME(btRigidBodyConstructionInfo_set_m_additionalDamping)( _ref(btRigidBody::btRigidBodyConstructionInfo)* _this, bool value ) {
	_unref(_this)->m_additionalDamping = (value);
}

HL_PRIM float HL_NAME(btRigidBodyConstructionInfo_get_m_additionalDampingFactor)( _ref(btRigidBody::btRigidBodyConstructionInfo)* _this ) {
	return _unref(_this)->m_additionalDampingFactor;
}
HL_PRIM void HL_NAME(btRigidBodyConstructionInfo_set_m_additionalDampingFactor)( _ref(btRigidBody::btRigidBodyConstructionInfo)* _this, float value ) {
	_unref(_this)->m_additionalDampingFactor = (value);
}

HL_PRIM float HL_NAME(btRigidBodyConstructionInfo_get_m_additionalLinearDampingThresholdSqr)( _ref(btRigidBody::btRigidBodyConstructionInfo)* _this ) {
	return _unref(_this)->m_additionalLinearDampingThresholdSqr;
}
HL_PRIM void HL_NAME(btRigidBodyConstructionInfo_set_m_additionalLinearDampingThresholdSqr)( _ref(btRigidBody::btRigidBodyConstructionInfo)* _this, float value ) {
	_unref(_this)->m_additionalLinearDampingThresholdSqr = (value);
}

HL_PRIM float HL_NAME(btRigidBodyConstructionInfo_get_m_additionalAngularDampingThresholdSqr)( _ref(btRigidBody::btRigidBodyConstructionInfo)* _this ) {
	return _unref(_this)->m_additionalAngularDampingThresholdSqr;
}
HL_PRIM void HL_NAME(btRigidBodyConstructionInfo_set_m_additionalAngularDampingThresholdSqr)( _ref(btRigidBody::btRigidBodyConstructionInfo)* _this, float value ) {
	_unref(_this)->m_additionalAngularDampingThresholdSqr = (value);
}

HL_PRIM float HL_NAME(btRigidBodyConstructionInfo_get_m_additionalAngularDampingFactor)( _ref(btRigidBody::btRigidBodyConstructionInfo)* _this ) {
	return _unref(_this)->m_additionalAngularDampingFactor;
}
HL_PRIM void HL_NAME(btRigidBodyConstructionInfo_set_m_additionalAngularDampingFactor)( _ref(btRigidBody::btRigidBodyConstructionInfo)* _this, float value ) {
	_unref(_this)->m_additionalAngularDampingFactor = (value);
}

HL_PRIM _ref(btRigidBody)* HL_NAME(btRigidBody_new1)(_ref(btRigidBody::btRigidBodyConstructionInfo)* constructionInfo) {
	return alloc_ref((new btRigidBody(*_unref(constructionInfo))),finalize_btRigidBody);
}
DEFINE_PRIM(_IDL, btRigidBody_new1, _IDL);

HL_PRIM _ref(btTransform)* HL_NAME(btRigidBody_getCenterOfMassTransform)(_ref(btRigidBody)* _this) {
	return alloc_ref(new btTransform(_unref(_this)->getCenterOfMassTransform()),finalize_btTransform);
}
DEFINE_PRIM(_IDL, btRigidBody_getCenterOfMassTransform, _IDL);

HL_PRIM void HL_NAME(btRigidBody_setCenterOfMassTransform)(_ref(btRigidBody)* _this, _ref(btTransform)* xform) {
	_unref(_this)->setCenterOfMassTransform(*_unref(xform));
}
DEFINE_PRIM(_VOID, btRigidBody_setCenterOfMassTransform, _IDL _IDL);

HL_PRIM void HL_NAME(btRigidBody_setSleepingThresholds)(_ref(btRigidBody)* _this, float linear, float angular) {
	_unref(_this)->setSleepingThresholds(linear, angular);
}
DEFINE_PRIM(_VOID, btRigidBody_setSleepingThresholds, _IDL _F32 _F32);

HL_PRIM void HL_NAME(btRigidBody_setDamping)(_ref(btRigidBody)* _this, float lin_damping, float ang_damping) {
	_unref(_this)->setDamping(lin_damping, ang_damping);
}
DEFINE_PRIM(_VOID, btRigidBody_setDamping, _IDL _F32 _F32);

HL_PRIM void HL_NAME(btRigidBody_setMassProps)(_ref(btRigidBody)* _this, float mass, _ref(btVector3)* inertia) {
	_unref(_this)->setMassProps(mass, *_unref(inertia));
}
DEFINE_PRIM(_VOID, btRigidBody_setMassProps, _IDL _F32 _IDL);

HL_PRIM void HL_NAME(btRigidBody_setLinearFactor)(_ref(btRigidBody)* _this, _ref(btVector3)* linearFactor) {
	_unref(_this)->setLinearFactor(*_unref(linearFactor));
}
DEFINE_PRIM(_VOID, btRigidBody_setLinearFactor, _IDL _IDL);

HL_PRIM void HL_NAME(btRigidBody_applyTorque)(_ref(btRigidBody)* _this, _ref(btVector3)* torque) {
	_unref(_this)->applyTorque(*_unref(torque));
}
DEFINE_PRIM(_VOID, btRigidBody_applyTorque, _IDL _IDL);

HL_PRIM void HL_NAME(btRigidBody_applyForce)(_ref(btRigidBody)* _this, _ref(btVector3)* force, _ref(btVector3)* rel_pos) {
	_unref(_this)->applyForce(*_unref(force), *_unref(rel_pos));
}
DEFINE_PRIM(_VOID, btRigidBody_applyForce, _IDL _IDL _IDL);

HL_PRIM void HL_NAME(btRigidBody_applyCentralForce)(_ref(btRigidBody)* _this, _ref(btVector3)* force) {
	_unref(_this)->applyCentralForce(*_unref(force));
}
DEFINE_PRIM(_VOID, btRigidBody_applyCentralForce, _IDL _IDL);

HL_PRIM void HL_NAME(btRigidBody_applyTorqueImpulse)(_ref(btRigidBody)* _this, _ref(btVector3)* torque) {
	_unref(_this)->applyTorqueImpulse(*_unref(torque));
}
DEFINE_PRIM(_VOID, btRigidBody_applyTorqueImpulse, _IDL _IDL);

HL_PRIM void HL_NAME(btRigidBody_applyImpulse)(_ref(btRigidBody)* _this, _ref(btVector3)* impulse, _ref(btVector3)* rel_pos) {
	_unref(_this)->applyImpulse(*_unref(impulse), *_unref(rel_pos));
}
DEFINE_PRIM(_VOID, btRigidBody_applyImpulse, _IDL _IDL _IDL);

HL_PRIM void HL_NAME(btRigidBody_applyCentralImpulse)(_ref(btRigidBody)* _this, _ref(btVector3)* impulse) {
	_unref(_this)->applyCentralImpulse(*_unref(impulse));
}
DEFINE_PRIM(_VOID, btRigidBody_applyCentralImpulse, _IDL _IDL);

HL_PRIM void HL_NAME(btRigidBody_updateInertiaTensor)(_ref(btRigidBody)* _this) {
	_unref(_this)->updateInertiaTensor();
}
DEFINE_PRIM(_VOID, btRigidBody_updateInertiaTensor, _IDL);

HL_PRIM _ref(btVector3)* HL_NAME(btRigidBody_getLinearVelocity)(_ref(btRigidBody)* _this) {
	return alloc_ref(new btVector3(_unref(_this)->getLinearVelocity()),finalize_btVector3);
}
DEFINE_PRIM(_IDL, btRigidBody_getLinearVelocity, _IDL);

HL_PRIM _ref(btVector3)* HL_NAME(btRigidBody_getAngularVelocity)(_ref(btRigidBody)* _this) {
	return alloc_ref(new btVector3(_unref(_this)->getAngularVelocity()),finalize_btVector3);
}
DEFINE_PRIM(_IDL, btRigidBody_getAngularVelocity, _IDL);

HL_PRIM void HL_NAME(btRigidBody_setLinearVelocity)(_ref(btRigidBody)* _this, _ref(btVector3)* lin_vel) {
	_unref(_this)->setLinearVelocity(*_unref(lin_vel));
}
DEFINE_PRIM(_VOID, btRigidBody_setLinearVelocity, _IDL _IDL);

HL_PRIM void HL_NAME(btRigidBody_setAngularVelocity)(_ref(btRigidBody)* _this, _ref(btVector3)* ang_vel) {
	_unref(_this)->setAngularVelocity(*_unref(ang_vel));
}
DEFINE_PRIM(_VOID, btRigidBody_setAngularVelocity, _IDL _IDL);

HL_PRIM _ref(btMotionState)* HL_NAME(btRigidBody_getMotionState)(_ref(btRigidBody)* _this) {
	return alloc_ref((_unref(_this)->getMotionState()),finalize_btMotionState);
}
DEFINE_PRIM(_IDL, btRigidBody_getMotionState, _IDL);

HL_PRIM void HL_NAME(btRigidBody_setMotionState)(_ref(btRigidBody)* _this, _ref(btMotionState)* motionState) {
	_unref(_this)->setMotionState(_unref(motionState));
}
DEFINE_PRIM(_VOID, btRigidBody_setMotionState, _IDL _IDL);

HL_PRIM void HL_NAME(btRigidBody_setAngularFactor)(_ref(btRigidBody)* _this, _ref(btVector3)* angularFactor) {
	_unref(_this)->setAngularFactor(*_unref(angularFactor));
}
DEFINE_PRIM(_VOID, btRigidBody_setAngularFactor, _IDL _IDL);

HL_PRIM _ref(btRigidBody)* HL_NAME(btRigidBody_upcast)(_ref(btRigidBody)* _this, _ref(btCollisionObject)* colObj) {
	return alloc_ref((_unref(_this)->upcast(_unref(colObj))),finalize_btRigidBody);
}
DEFINE_PRIM(_IDL, btRigidBody_upcast, _IDL _IDL);

HL_PRIM void HL_NAME(btRigidBody_getAabb)(_ref(btRigidBody)* _this, _ref(btVector3)* aabbMin, _ref(btVector3)* aabbMax) {
	_unref(_this)->getAabb(*_unref(aabbMin), *_unref(aabbMax));
}
DEFINE_PRIM(_VOID, btRigidBody_getAabb, _IDL _IDL _IDL);

HL_PRIM _ref(btConstraintSetting)* HL_NAME(btConstraintSetting_new0)() {
	return alloc_ref((new btConstraintSetting()),finalize_btConstraintSetting);
}
DEFINE_PRIM(_IDL, btConstraintSetting_new0,);

HL_PRIM float HL_NAME(btConstraintSetting_get_m_tau)( _ref(btConstraintSetting)* _this ) {
	return _unref(_this)->m_tau;
}
HL_PRIM void HL_NAME(btConstraintSetting_set_m_tau)( _ref(btConstraintSetting)* _this, float value ) {
	_unref(_this)->m_tau = (value);
}

HL_PRIM float HL_NAME(btConstraintSetting_get_m_damping)( _ref(btConstraintSetting)* _this ) {
	return _unref(_this)->m_damping;
}
HL_PRIM void HL_NAME(btConstraintSetting_set_m_damping)( _ref(btConstraintSetting)* _this, float value ) {
	_unref(_this)->m_damping = (value);
}

HL_PRIM float HL_NAME(btConstraintSetting_get_m_impulseClamp)( _ref(btConstraintSetting)* _this ) {
	return _unref(_this)->m_impulseClamp;
}
HL_PRIM void HL_NAME(btConstraintSetting_set_m_impulseClamp)( _ref(btConstraintSetting)* _this, float value ) {
	_unref(_this)->m_impulseClamp = (value);
}

HL_PRIM void HL_NAME(btTypedConstraint_enableFeedback)(_ref(btTypedConstraint)* _this, bool needsFeedback) {
	_unref(_this)->enableFeedback(needsFeedback);
}
DEFINE_PRIM(_VOID, btTypedConstraint_enableFeedback, _IDL _BOOL);

HL_PRIM float HL_NAME(btTypedConstraint_getBreakingImpulseThreshold)(_ref(btTypedConstraint)* _this) {
	return _unref(_this)->getBreakingImpulseThreshold();
}
DEFINE_PRIM(_F32, btTypedConstraint_getBreakingImpulseThreshold, _IDL);

HL_PRIM void HL_NAME(btTypedConstraint_setBreakingImpulseThreshold)(_ref(btTypedConstraint)* _this, float threshold) {
	_unref(_this)->setBreakingImpulseThreshold(threshold);
}
DEFINE_PRIM(_VOID, btTypedConstraint_setBreakingImpulseThreshold, _IDL _F32);

HL_PRIM float HL_NAME(btTypedConstraint_getParam)(_ref(btTypedConstraint)* _this, int num, int axis) {
	return _unref(_this)->getParam(num, axis);
}
DEFINE_PRIM(_F32, btTypedConstraint_getParam, _IDL _I32 _I32);

HL_PRIM void HL_NAME(btTypedConstraint_setParam)(_ref(btTypedConstraint)* _this, int num, float value, int axis) {
	_unref(_this)->setParam(num, value, axis);
}
DEFINE_PRIM(_VOID, btTypedConstraint_setParam, _IDL _I32 _F32 _I32);

HL_PRIM _ref(btPoint2PointConstraint)* HL_NAME(btPoint2PointConstraint_new4)(_ref(btRigidBody)* rbA, _ref(btRigidBody)* rbB, _ref(btVector3)* pivotInA, _ref(btVector3)* pivotInB) {
	return alloc_ref((new btPoint2PointConstraint(*_unref(rbA), *_unref(rbB), *_unref(pivotInA), *_unref(pivotInB))),finalize_btPoint2PointConstraint);
}
DEFINE_PRIM(_IDL, btPoint2PointConstraint_new4, _IDL _IDL _IDL _IDL);

HL_PRIM _ref(btPoint2PointConstraint)* HL_NAME(btPoint2PointConstraint_new2)(_ref(btRigidBody)* rbA, _ref(btVector3)* pivotInA) {
	return alloc_ref((new btPoint2PointConstraint(*_unref(rbA), *_unref(pivotInA))),finalize_btPoint2PointConstraint);
}
DEFINE_PRIM(_IDL, btPoint2PointConstraint_new2, _IDL _IDL);

HL_PRIM void HL_NAME(btPoint2PointConstraint_setPivotA)(_ref(btPoint2PointConstraint)* _this, _ref(btVector3)* pivotA) {
	_unref(_this)->setPivotA(*_unref(pivotA));
}
DEFINE_PRIM(_VOID, btPoint2PointConstraint_setPivotA, _IDL _IDL);

HL_PRIM void HL_NAME(btPoint2PointConstraint_setPivotB)(_ref(btPoint2PointConstraint)* _this, _ref(btVector3)* pivotB) {
	_unref(_this)->setPivotB(*_unref(pivotB));
}
DEFINE_PRIM(_VOID, btPoint2PointConstraint_setPivotB, _IDL _IDL);

HL_PRIM _ref(btVector3)* HL_NAME(btPoint2PointConstraint_getPivotInA)(_ref(btPoint2PointConstraint)* _this) {
	return alloc_ref(new btVector3(_unref(_this)->getPivotInA()),finalize_btVector3);
}
DEFINE_PRIM(_IDL, btPoint2PointConstraint_getPivotInA, _IDL);

HL_PRIM _ref(btVector3)* HL_NAME(btPoint2PointConstraint_getPivotInB)(_ref(btPoint2PointConstraint)* _this) {
	return alloc_ref(new btVector3(_unref(_this)->getPivotInB()),finalize_btVector3);
}
DEFINE_PRIM(_IDL, btPoint2PointConstraint_getPivotInB, _IDL);

HL_PRIM _ref(btConstraintSetting)* HL_NAME(btPoint2PointConstraint_get_m_setting)( _ref(btPoint2PointConstraint)* _this ) {
	return alloc_ref(new btConstraintSetting(_unref(_this)->m_setting),finalize_btConstraintSetting);
}
HL_PRIM void HL_NAME(btPoint2PointConstraint_set_m_setting)( _ref(btPoint2PointConstraint)* _this, _ref(btConstraintSetting)* value ) {
	_unref(_this)->m_setting = *_unref(value);
}

HL_PRIM _ref(btGeneric6DofConstraint)* HL_NAME(btGeneric6DofConstraint_new5)(_ref(btRigidBody)* rbA, _ref(btRigidBody)* rbB, _ref(btTransform)* frameInA, _ref(btTransform)* frameInB, bool useLinearFrameReferenceFrameA) {
	return alloc_ref((new btGeneric6DofConstraint(*_unref(rbA), *_unref(rbB), *_unref(frameInA), *_unref(frameInB), useLinearFrameReferenceFrameA)),finalize_btGeneric6DofConstraint);
}
DEFINE_PRIM(_IDL, btGeneric6DofConstraint_new5, _IDL _IDL _IDL _IDL _BOOL);

HL_PRIM _ref(btGeneric6DofConstraint)* HL_NAME(btGeneric6DofConstraint_new3)(_ref(btRigidBody)* rbB, _ref(btTransform)* frameInB, bool useLinearFrameReferenceFrameB) {
	return alloc_ref((new btGeneric6DofConstraint(*_unref(rbB), *_unref(frameInB), useLinearFrameReferenceFrameB)),finalize_btGeneric6DofConstraint);
}
DEFINE_PRIM(_IDL, btGeneric6DofConstraint_new3, _IDL _IDL _BOOL);

HL_PRIM void HL_NAME(btGeneric6DofConstraint_setLinearLowerLimit)(_ref(btGeneric6DofConstraint)* _this, _ref(btVector3)* linearLower) {
	_unref(_this)->setLinearLowerLimit(*_unref(linearLower));
}
DEFINE_PRIM(_VOID, btGeneric6DofConstraint_setLinearLowerLimit, _IDL _IDL);

HL_PRIM void HL_NAME(btGeneric6DofConstraint_setLinearUpperLimit)(_ref(btGeneric6DofConstraint)* _this, _ref(btVector3)* linearUpper) {
	_unref(_this)->setLinearUpperLimit(*_unref(linearUpper));
}
DEFINE_PRIM(_VOID, btGeneric6DofConstraint_setLinearUpperLimit, _IDL _IDL);

HL_PRIM void HL_NAME(btGeneric6DofConstraint_setAngularLowerLimit)(_ref(btGeneric6DofConstraint)* _this, _ref(btVector3)* angularLower) {
	_unref(_this)->setAngularLowerLimit(*_unref(angularLower));
}
DEFINE_PRIM(_VOID, btGeneric6DofConstraint_setAngularLowerLimit, _IDL _IDL);

HL_PRIM void HL_NAME(btGeneric6DofConstraint_setAngularUpperLimit)(_ref(btGeneric6DofConstraint)* _this, _ref(btVector3)* angularUpper) {
	_unref(_this)->setAngularUpperLimit(*_unref(angularUpper));
}
DEFINE_PRIM(_VOID, btGeneric6DofConstraint_setAngularUpperLimit, _IDL _IDL);

HL_PRIM _ref(btGeneric6DofSpringConstraint)* HL_NAME(btGeneric6DofSpringConstraint_new5)(_ref(btRigidBody)* rbA, _ref(btRigidBody)* rbB, _ref(btTransform)* frameInA, _ref(btTransform)* frameInB, bool useLinearFrameReferenceFrameA) {
	return alloc_ref((new btGeneric6DofSpringConstraint(*_unref(rbA), *_unref(rbB), *_unref(frameInA), *_unref(frameInB), useLinearFrameReferenceFrameA)),finalize_btGeneric6DofSpringConstraint);
}
DEFINE_PRIM(_IDL, btGeneric6DofSpringConstraint_new5, _IDL _IDL _IDL _IDL _BOOL);

HL_PRIM _ref(btGeneric6DofSpringConstraint)* HL_NAME(btGeneric6DofSpringConstraint_new3)(_ref(btRigidBody)* rbB, _ref(btTransform)* frameInB, bool useLinearFrameReferenceFrameB) {
	return alloc_ref((new btGeneric6DofSpringConstraint(*_unref(rbB), *_unref(frameInB), useLinearFrameReferenceFrameB)),finalize_btGeneric6DofSpringConstraint);
}
DEFINE_PRIM(_IDL, btGeneric6DofSpringConstraint_new3, _IDL _IDL _BOOL);

HL_PRIM void HL_NAME(btGeneric6DofSpringConstraint_enableSpring)(_ref(btGeneric6DofSpringConstraint)* _this, int index, bool onOff) {
	_unref(_this)->enableSpring(index, onOff);
}
DEFINE_PRIM(_VOID, btGeneric6DofSpringConstraint_enableSpring, _IDL _I32 _BOOL);

HL_PRIM void HL_NAME(btGeneric6DofSpringConstraint_setStiffness)(_ref(btGeneric6DofSpringConstraint)* _this, int index, float stiffness) {
	_unref(_this)->setStiffness(index, stiffness);
}
DEFINE_PRIM(_VOID, btGeneric6DofSpringConstraint_setStiffness, _IDL _I32 _F32);

HL_PRIM void HL_NAME(btGeneric6DofSpringConstraint_setDamping)(_ref(btGeneric6DofSpringConstraint)* _this, int index, float damping) {
	_unref(_this)->setDamping(index, damping);
}
DEFINE_PRIM(_VOID, btGeneric6DofSpringConstraint_setDamping, _IDL _I32 _F32);

HL_PRIM _ref(btSequentialImpulseConstraintSolver)* HL_NAME(btSequentialImpulseConstraintSolver_new0)() {
	return alloc_ref((new btSequentialImpulseConstraintSolver()),finalize_btSequentialImpulseConstraintSolver);
}
DEFINE_PRIM(_IDL, btSequentialImpulseConstraintSolver_new0,);

HL_PRIM _ref(btConeTwistConstraint)* HL_NAME(btConeTwistConstraint_new4)(_ref(btRigidBody)* rbA, _ref(btRigidBody)* rbB, _ref(btTransform)* rbAFrame, _ref(btTransform)* rbBFrame) {
	return alloc_ref((new btConeTwistConstraint(*_unref(rbA), *_unref(rbB), *_unref(rbAFrame), *_unref(rbBFrame))),finalize_btConeTwistConstraint);
}
DEFINE_PRIM(_IDL, btConeTwistConstraint_new4, _IDL _IDL _IDL _IDL);

HL_PRIM _ref(btConeTwistConstraint)* HL_NAME(btConeTwistConstraint_new2)(_ref(btRigidBody)* rbA, _ref(btTransform)* rbAFrame) {
	return alloc_ref((new btConeTwistConstraint(*_unref(rbA), *_unref(rbAFrame))),finalize_btConeTwistConstraint);
}
DEFINE_PRIM(_IDL, btConeTwistConstraint_new2, _IDL _IDL);

HL_PRIM void HL_NAME(btConeTwistConstraint_setLimit)(_ref(btConeTwistConstraint)* _this, int limitIndex, float limitValue) {
	_unref(_this)->setLimit(limitIndex, limitValue);
}
DEFINE_PRIM(_VOID, btConeTwistConstraint_setLimit, _IDL _I32 _F32);

HL_PRIM void HL_NAME(btConeTwistConstraint_setAngularOnly)(_ref(btConeTwistConstraint)* _this, bool angularOnly) {
	_unref(_this)->setAngularOnly(angularOnly);
}
DEFINE_PRIM(_VOID, btConeTwistConstraint_setAngularOnly, _IDL _BOOL);

HL_PRIM void HL_NAME(btConeTwistConstraint_setDamping)(_ref(btConeTwistConstraint)* _this, float damping) {
	_unref(_this)->setDamping(damping);
}
DEFINE_PRIM(_VOID, btConeTwistConstraint_setDamping, _IDL _F32);

HL_PRIM void HL_NAME(btConeTwistConstraint_enableMotor)(_ref(btConeTwistConstraint)* _this, bool b) {
	_unref(_this)->enableMotor(b);
}
DEFINE_PRIM(_VOID, btConeTwistConstraint_enableMotor, _IDL _BOOL);

HL_PRIM void HL_NAME(btConeTwistConstraint_setMaxMotorImpulse)(_ref(btConeTwistConstraint)* _this, float maxMotorImpulse) {
	_unref(_this)->setMaxMotorImpulse(maxMotorImpulse);
}
DEFINE_PRIM(_VOID, btConeTwistConstraint_setMaxMotorImpulse, _IDL _F32);

HL_PRIM void HL_NAME(btConeTwistConstraint_setMaxMotorImpulseNormalized)(_ref(btConeTwistConstraint)* _this, float maxMotorImpulse) {
	_unref(_this)->setMaxMotorImpulseNormalized(maxMotorImpulse);
}
DEFINE_PRIM(_VOID, btConeTwistConstraint_setMaxMotorImpulseNormalized, _IDL _F32);

HL_PRIM void HL_NAME(btConeTwistConstraint_setMotorTarget)(_ref(btConeTwistConstraint)* _this, _ref(btQuaternion)* q) {
	_unref(_this)->setMotorTarget(*_unref(q));
}
DEFINE_PRIM(_VOID, btConeTwistConstraint_setMotorTarget, _IDL _IDL);

HL_PRIM void HL_NAME(btConeTwistConstraint_setMotorTargetInConstraintSpace)(_ref(btConeTwistConstraint)* _this, _ref(btQuaternion)* q) {
	_unref(_this)->setMotorTargetInConstraintSpace(*_unref(q));
}
DEFINE_PRIM(_VOID, btConeTwistConstraint_setMotorTargetInConstraintSpace, _IDL _IDL);

HL_PRIM _ref(btHingeConstraint)* HL_NAME(btHingeConstraint_new7)(_ref(btRigidBody)* rbA, _ref(btRigidBody)* rbB, _ref(btVector3)* pivotInA, _ref(btVector3)* pivotInB, _ref(btVector3)* axisInA, _ref(btVector3)* axisInB, vdynamic* useReferenceFrameA) {
	if( !useReferenceFrameA )
		return alloc_ref((new btHingeConstraint(*_unref(rbA), *_unref(rbB), *_unref(pivotInA), *_unref(pivotInB), *_unref(axisInA), *_unref(axisInB))),finalize_btHingeConstraint);
	else
		return alloc_ref((new btHingeConstraint(*_unref(rbA), *_unref(rbB), *_unref(pivotInA), *_unref(pivotInB), *_unref(axisInA), *_unref(axisInB), useReferenceFrameA->v.b)),finalize_btHingeConstraint);
}
DEFINE_PRIM(_IDL, btHingeConstraint_new7, _IDL _IDL _IDL _IDL _IDL _IDL _NULL(_BOOL));

HL_PRIM _ref(btHingeConstraint)* HL_NAME(btHingeConstraint_new5)(_ref(btRigidBody)* rbA, _ref(btRigidBody)* rbB, _ref(btTransform)* rbAFrame, _ref(btTransform)* rbBFrame, vdynamic* useReferenceFrameA) {
	if( !useReferenceFrameA )
		return alloc_ref((new btHingeConstraint(*_unref(rbA), *_unref(rbB), *_unref(rbAFrame), *_unref(rbBFrame))),finalize_btHingeConstraint);
	else
		return alloc_ref((new btHingeConstraint(*_unref(rbA), *_unref(rbB), *_unref(rbAFrame), *_unref(rbBFrame), useReferenceFrameA->v.b)),finalize_btHingeConstraint);
}
DEFINE_PRIM(_IDL, btHingeConstraint_new5, _IDL _IDL _IDL _IDL _NULL(_BOOL));

HL_PRIM _ref(btHingeConstraint)* HL_NAME(btHingeConstraint_new3)(_ref(btRigidBody)* rbA, _ref(btTransform)* rbAFrame, vdynamic* useReferenceFrameA) {
	if( !useReferenceFrameA )
		return alloc_ref((new btHingeConstraint(*_unref(rbA), *_unref(rbAFrame))),finalize_btHingeConstraint);
	else
		return alloc_ref((new btHingeConstraint(*_unref(rbA), *_unref(rbAFrame), useReferenceFrameA->v.b)),finalize_btHingeConstraint);
}
DEFINE_PRIM(_IDL, btHingeConstraint_new3, _IDL _IDL _NULL(_BOOL));

HL_PRIM void HL_NAME(btHingeConstraint_setLimit)(_ref(btHingeConstraint)* _this, float low, float high, float softness, float biasFactor, vdynamic* relaxationFactor) {
	if( !relaxationFactor )
		_unref(_this)->setLimit(low, high, softness, biasFactor);
	else
		_unref(_this)->setLimit(low, high, softness, biasFactor, relaxationFactor->v.f);
}
DEFINE_PRIM(_VOID, btHingeConstraint_setLimit, _IDL _F32 _F32 _F32 _F32 _NULL(_F32));

HL_PRIM void HL_NAME(btHingeConstraint_enableAngularMotor)(_ref(btHingeConstraint)* _this, bool enableMotor, float targetVelocity, float maxMotorImpulse) {
	_unref(_this)->enableAngularMotor(enableMotor, targetVelocity, maxMotorImpulse);
}
DEFINE_PRIM(_VOID, btHingeConstraint_enableAngularMotor, _IDL _BOOL _F32 _F32);

HL_PRIM void HL_NAME(btHingeConstraint_setAngularOnly)(_ref(btHingeConstraint)* _this, bool angularOnly) {
	_unref(_this)->setAngularOnly(angularOnly);
}
DEFINE_PRIM(_VOID, btHingeConstraint_setAngularOnly, _IDL _BOOL);

HL_PRIM void HL_NAME(btHingeConstraint_enableMotor)(_ref(btHingeConstraint)* _this, bool enableMotor) {
	_unref(_this)->enableMotor(enableMotor);
}
DEFINE_PRIM(_VOID, btHingeConstraint_enableMotor, _IDL _BOOL);

HL_PRIM void HL_NAME(btHingeConstraint_setMaxMotorImpulse)(_ref(btHingeConstraint)* _this, float maxMotorImpulse) {
	_unref(_this)->setMaxMotorImpulse(maxMotorImpulse);
}
DEFINE_PRIM(_VOID, btHingeConstraint_setMaxMotorImpulse, _IDL _F32);

HL_PRIM void HL_NAME(btHingeConstraint_setMotorTarget)(_ref(btHingeConstraint)* _this, float targetAngle, float dt) {
	_unref(_this)->setMotorTarget(targetAngle, dt);
}
DEFINE_PRIM(_VOID, btHingeConstraint_setMotorTarget, _IDL _F32 _F32);

HL_PRIM _ref(btSliderConstraint)* HL_NAME(btSliderConstraint_new5)(_ref(btRigidBody)* rbA, _ref(btRigidBody)* rbB, _ref(btTransform)* frameInA, _ref(btTransform)* frameInB, bool useLinearReferenceFrameA) {
	return alloc_ref((new btSliderConstraint(*_unref(rbA), *_unref(rbB), *_unref(frameInA), *_unref(frameInB), useLinearReferenceFrameA)),finalize_btSliderConstraint);
}
DEFINE_PRIM(_IDL, btSliderConstraint_new5, _IDL _IDL _IDL _IDL _BOOL);

HL_PRIM _ref(btSliderConstraint)* HL_NAME(btSliderConstraint_new3)(_ref(btRigidBody)* rbB, _ref(btTransform)* frameInB, bool useLinearReferenceFrameA) {
	return alloc_ref((new btSliderConstraint(*_unref(rbB), *_unref(frameInB), useLinearReferenceFrameA)),finalize_btSliderConstraint);
}
DEFINE_PRIM(_IDL, btSliderConstraint_new3, _IDL _IDL _BOOL);

HL_PRIM void HL_NAME(btSliderConstraint_setLowerLinLimit)(_ref(btSliderConstraint)* _this, float lowerLimit) {
	_unref(_this)->setLowerLinLimit(lowerLimit);
}
DEFINE_PRIM(_VOID, btSliderConstraint_setLowerLinLimit, _IDL _F32);

HL_PRIM void HL_NAME(btSliderConstraint_setUpperLinLimit)(_ref(btSliderConstraint)* _this, float upperLimit) {
	_unref(_this)->setUpperLinLimit(upperLimit);
}
DEFINE_PRIM(_VOID, btSliderConstraint_setUpperLinLimit, _IDL _F32);

HL_PRIM void HL_NAME(btSliderConstraint_setLowerAngLimit)(_ref(btSliderConstraint)* _this, float lowerAngLimit) {
	_unref(_this)->setLowerAngLimit(lowerAngLimit);
}
DEFINE_PRIM(_VOID, btSliderConstraint_setLowerAngLimit, _IDL _F32);

HL_PRIM void HL_NAME(btSliderConstraint_setUpperAngLimit)(_ref(btSliderConstraint)* _this, float upperAngLimit) {
	_unref(_this)->setUpperAngLimit(upperAngLimit);
}
DEFINE_PRIM(_VOID, btSliderConstraint_setUpperAngLimit, _IDL _F32);

HL_PRIM _ref(btFixedConstraint)* HL_NAME(btFixedConstraint_new4)(_ref(btRigidBody)* rbA, _ref(btRigidBody)* rbB, _ref(btTransform)* frameInA, _ref(btTransform)* frameInB) {
	return alloc_ref((new btFixedConstraint(*_unref(rbA), *_unref(rbB), *_unref(frameInA), *_unref(frameInB))),finalize_btFixedConstraint);
}
DEFINE_PRIM(_IDL, btFixedConstraint_new4, _IDL _IDL _IDL _IDL);

HL_PRIM float HL_NAME(btDispatcherInfo_get_m_timeStep)( _ref(btDispatcherInfo)* _this ) {
	return _unref(_this)->m_timeStep;
}
HL_PRIM void HL_NAME(btDispatcherInfo_set_m_timeStep)( _ref(btDispatcherInfo)* _this, float value ) {
	_unref(_this)->m_timeStep = (value);
}

HL_PRIM int HL_NAME(btDispatcherInfo_get_m_stepCount)( _ref(btDispatcherInfo)* _this ) {
	return _unref(_this)->m_stepCount;
}
HL_PRIM void HL_NAME(btDispatcherInfo_set_m_stepCount)( _ref(btDispatcherInfo)* _this, int value ) {
	_unref(_this)->m_stepCount = (value);
}

HL_PRIM int HL_NAME(btDispatcherInfo_get_m_dispatchFunc)( _ref(btDispatcherInfo)* _this ) {
	return _unref(_this)->m_dispatchFunc;
}
HL_PRIM void HL_NAME(btDispatcherInfo_set_m_dispatchFunc)( _ref(btDispatcherInfo)* _this, int value ) {
	_unref(_this)->m_dispatchFunc = (value);
}

HL_PRIM float HL_NAME(btDispatcherInfo_get_m_timeOfImpact)( _ref(btDispatcherInfo)* _this ) {
	return _unref(_this)->m_timeOfImpact;
}
HL_PRIM void HL_NAME(btDispatcherInfo_set_m_timeOfImpact)( _ref(btDispatcherInfo)* _this, float value ) {
	_unref(_this)->m_timeOfImpact = (value);
}

HL_PRIM bool HL_NAME(btDispatcherInfo_get_m_useContinuous)( _ref(btDispatcherInfo)* _this ) {
	return _unref(_this)->m_useContinuous;
}
HL_PRIM void HL_NAME(btDispatcherInfo_set_m_useContinuous)( _ref(btDispatcherInfo)* _this, bool value ) {
	_unref(_this)->m_useContinuous = (value);
}

HL_PRIM bool HL_NAME(btDispatcherInfo_get_m_enableSatConvex)( _ref(btDispatcherInfo)* _this ) {
	return _unref(_this)->m_enableSatConvex;
}
HL_PRIM void HL_NAME(btDispatcherInfo_set_m_enableSatConvex)( _ref(btDispatcherInfo)* _this, bool value ) {
	_unref(_this)->m_enableSatConvex = (value);
}

HL_PRIM bool HL_NAME(btDispatcherInfo_get_m_enableSPU)( _ref(btDispatcherInfo)* _this ) {
	return _unref(_this)->m_enableSPU;
}
HL_PRIM void HL_NAME(btDispatcherInfo_set_m_enableSPU)( _ref(btDispatcherInfo)* _this, bool value ) {
	_unref(_this)->m_enableSPU = (value);
}

HL_PRIM bool HL_NAME(btDispatcherInfo_get_m_useEpa)( _ref(btDispatcherInfo)* _this ) {
	return _unref(_this)->m_useEpa;
}
HL_PRIM void HL_NAME(btDispatcherInfo_set_m_useEpa)( _ref(btDispatcherInfo)* _this, bool value ) {
	_unref(_this)->m_useEpa = (value);
}

HL_PRIM float HL_NAME(btDispatcherInfo_get_m_allowedCcdPenetration)( _ref(btDispatcherInfo)* _this ) {
	return _unref(_this)->m_allowedCcdPenetration;
}
HL_PRIM void HL_NAME(btDispatcherInfo_set_m_allowedCcdPenetration)( _ref(btDispatcherInfo)* _this, float value ) {
	_unref(_this)->m_allowedCcdPenetration = (value);
}

HL_PRIM bool HL_NAME(btDispatcherInfo_get_m_useConvexConservativeDistanceUtil)( _ref(btDispatcherInfo)* _this ) {
	return _unref(_this)->m_useConvexConservativeDistanceUtil;
}
HL_PRIM void HL_NAME(btDispatcherInfo_set_m_useConvexConservativeDistanceUtil)( _ref(btDispatcherInfo)* _this, bool value ) {
	_unref(_this)->m_useConvexConservativeDistanceUtil = (value);
}

HL_PRIM float HL_NAME(btDispatcherInfo_get_m_convexConservativeDistanceThreshold)( _ref(btDispatcherInfo)* _this ) {
	return _unref(_this)->m_convexConservativeDistanceThreshold;
}
HL_PRIM void HL_NAME(btDispatcherInfo_set_m_convexConservativeDistanceThreshold)( _ref(btDispatcherInfo)* _this, float value ) {
	_unref(_this)->m_convexConservativeDistanceThreshold = (value);
}

HL_PRIM _ref(btDispatcher)* HL_NAME(btCollisionWorld_getDispatcher)(_ref(btCollisionWorld)* _this) {
	return alloc_ref((_unref(_this)->getDispatcher()),finalize_btDispatcher);
}
DEFINE_PRIM(_IDL, btCollisionWorld_getDispatcher, _IDL);

HL_PRIM void HL_NAME(btCollisionWorld_rayTest)(_ref(btCollisionWorld)* _this, _ref(btVector3)* rayFromWorld, _ref(btVector3)* rayToWorld, _ref(btCollisionWorld::RayResultCallback)* resultCallback) {
	_unref(_this)->rayTest(*_unref(rayFromWorld), *_unref(rayToWorld), *_unref(resultCallback));
}
DEFINE_PRIM(_VOID, btCollisionWorld_rayTest, _IDL _IDL _IDL _IDL);

HL_PRIM _ref(btOverlappingPairCache)* HL_NAME(btCollisionWorld_getPairCache)(_ref(btCollisionWorld)* _this) {
	return alloc_ref((_unref(_this)->getPairCache()),finalize_btOverlappingPairCache);
}
DEFINE_PRIM(_IDL, btCollisionWorld_getPairCache, _IDL);

HL_PRIM _ref(btDispatcherInfo)* HL_NAME(btCollisionWorld_getDispatchInfo)(_ref(btCollisionWorld)* _this) {
	return alloc_ref(new btDispatcherInfo(_unref(_this)->getDispatchInfo()),finalize_btDispatcherInfo);
}
DEFINE_PRIM(_IDL, btCollisionWorld_getDispatchInfo, _IDL);

HL_PRIM void HL_NAME(btCollisionWorld_addCollisionObject)(_ref(btCollisionWorld)* _this, _ref(btCollisionObject)* collisionObject, vdynamic* collisionFilterGroup, vdynamic* collisionFilterMask) {
	if( !collisionFilterGroup )
		_unref(_this)->addCollisionObject(_unref(collisionObject));
	else
	if( !collisionFilterMask )
		_unref(_this)->addCollisionObject(_unref(collisionObject), collisionFilterGroup->v.ui16);
	else
		_unref(_this)->addCollisionObject(_unref(collisionObject), collisionFilterGroup->v.ui16, collisionFilterMask->v.ui16);
}
DEFINE_PRIM(_VOID, btCollisionWorld_addCollisionObject, _IDL _IDL _NULL(_I16) _NULL(_I16));

HL_PRIM _ref(btBroadphaseInterface)* HL_NAME(btCollisionWorld_getBroadphase)(_ref(btCollisionWorld)* _this) {
	return alloc_ref_const((_unref(_this)->getBroadphase()),finalize_btBroadphaseInterface);
}
DEFINE_PRIM(_IDL, btCollisionWorld_getBroadphase, _IDL);

HL_PRIM void HL_NAME(btCollisionWorld_convexSweepTest)(_ref(btCollisionWorld)* _this, _ref(btConvexShape)* castShape, _ref(btTransform)* from, _ref(btTransform)* to, _ref(btCollisionWorld::ConvexResultCallback)* resultCallback, float allowedCcdPenetration) {
	_unref(_this)->convexSweepTest(_unref(castShape), *_unref(from), *_unref(to), *_unref(resultCallback), allowedCcdPenetration);
}
DEFINE_PRIM(_VOID, btCollisionWorld_convexSweepTest, _IDL _IDL _IDL _IDL _IDL _F32);

HL_PRIM void HL_NAME(btCollisionWorld_contactPairTest)(_ref(btCollisionWorld)* _this, _ref(btCollisionObject)* colObjA, _ref(btCollisionObject)* colObjB, _ref(btCollisionWorld::ContactResultCallback)* resultCallback) {
	_unref(_this)->contactPairTest(_unref(colObjA), _unref(colObjB), *_unref(resultCallback));
}
DEFINE_PRIM(_VOID, btCollisionWorld_contactPairTest, _IDL _IDL _IDL _IDL);

HL_PRIM void HL_NAME(btCollisionWorld_contactTest)(_ref(btCollisionWorld)* _this, _ref(btCollisionObject)* colObj, _ref(btCollisionWorld::ContactResultCallback)* resultCallback) {
	_unref(_this)->contactTest(_unref(colObj), *_unref(resultCallback));
}
DEFINE_PRIM(_VOID, btCollisionWorld_contactTest, _IDL _IDL _IDL);

HL_PRIM int HL_NAME(btContactSolverInfo_get_m_splitImpulse)( _ref(btContactSolverInfo)* _this ) {
	return _unref(_this)->m_splitImpulse;
}
HL_PRIM void HL_NAME(btContactSolverInfo_set_m_splitImpulse)( _ref(btContactSolverInfo)* _this, int value ) {
	_unref(_this)->m_splitImpulse = (value);
}

HL_PRIM int HL_NAME(btContactSolverInfo_get_m_splitImpulsePenetrationThreshold)( _ref(btContactSolverInfo)* _this ) {
	return _unref(_this)->m_splitImpulsePenetrationThreshold;
}
HL_PRIM void HL_NAME(btContactSolverInfo_set_m_splitImpulsePenetrationThreshold)( _ref(btContactSolverInfo)* _this, int value ) {
	_unref(_this)->m_splitImpulsePenetrationThreshold = (value);
}

HL_PRIM int HL_NAME(btContactSolverInfo_get_m_numIterations)( _ref(btContactSolverInfo)* _this ) {
	return _unref(_this)->m_numIterations;
}
HL_PRIM void HL_NAME(btContactSolverInfo_set_m_numIterations)( _ref(btContactSolverInfo)* _this, int value ) {
	_unref(_this)->m_numIterations = (value);
}

HL_PRIM void HL_NAME(btDynamicsWorld_addAction)(_ref(btDynamicsWorld)* _this, _ref(btActionInterface)* action) {
	_unref(_this)->addAction(_unref(action));
}
DEFINE_PRIM(_VOID, btDynamicsWorld_addAction, _IDL _IDL);

HL_PRIM void HL_NAME(btDynamicsWorld_removeAction)(_ref(btDynamicsWorld)* _this, _ref(btActionInterface)* action) {
	_unref(_this)->removeAction(_unref(action));
}
DEFINE_PRIM(_VOID, btDynamicsWorld_removeAction, _IDL _IDL);

HL_PRIM _ref(btContactSolverInfo)* HL_NAME(btDynamicsWorld_getSolverInfo)(_ref(btDynamicsWorld)* _this) {
	return alloc_ref(new btContactSolverInfo(_unref(_this)->getSolverInfo()),finalize_btContactSolverInfo);
}
DEFINE_PRIM(_IDL, btDynamicsWorld_getSolverInfo, _IDL);

HL_PRIM _ref(btDiscreteDynamicsWorld)* HL_NAME(btDiscreteDynamicsWorld_new4)(_ref(btDispatcher)* dispatcher, _ref(btBroadphaseInterface)* pairCache, _ref(btConstraintSolver)* constraintSolver, _ref(btCollisionConfiguration)* collisionConfiguration) {
	return alloc_ref((new btDiscreteDynamicsWorld(_unref(dispatcher), _unref(pairCache), _unref(constraintSolver), _unref(collisionConfiguration))),finalize_btDiscreteDynamicsWorld);
}
DEFINE_PRIM(_IDL, btDiscreteDynamicsWorld_new4, _IDL _IDL _IDL _IDL);

HL_PRIM void HL_NAME(btDiscreteDynamicsWorld_setGravity)(_ref(btDiscreteDynamicsWorld)* _this, _ref(btVector3)* gravity) {
	_unref(_this)->setGravity(*_unref(gravity));
}
DEFINE_PRIM(_VOID, btDiscreteDynamicsWorld_setGravity, _IDL _IDL);

HL_PRIM _ref(btVector3)* HL_NAME(btDiscreteDynamicsWorld_getGravity)(_ref(btDiscreteDynamicsWorld)* _this) {
	return alloc_ref(new btVector3(_unref(_this)->getGravity()),finalize_btVector3);
}
DEFINE_PRIM(_IDL, btDiscreteDynamicsWorld_getGravity, _IDL);

HL_PRIM void HL_NAME(btDiscreteDynamicsWorld_addRigidBody)(_ref(btDiscreteDynamicsWorld)* _this, _ref(btRigidBody)* body, short group, short mask) {
	_unref(_this)->addRigidBody(_unref(body), group, mask);
}
DEFINE_PRIM(_VOID, btDiscreteDynamicsWorld_addRigidBody, _IDL _IDL _I16 _I16);

HL_PRIM void HL_NAME(btDiscreteDynamicsWorld_removeRigidBody)(_ref(btDiscreteDynamicsWorld)* _this, _ref(btRigidBody)* body) {
	_unref(_this)->removeRigidBody(_unref(body));
}
DEFINE_PRIM(_VOID, btDiscreteDynamicsWorld_removeRigidBody, _IDL _IDL);

HL_PRIM void HL_NAME(btDiscreteDynamicsWorld_addConstraint)(_ref(btDiscreteDynamicsWorld)* _this, _ref(btTypedConstraint)* constraint, vdynamic* disableCollisionsBetweenLinkedBodies) {
	if( !disableCollisionsBetweenLinkedBodies )
		_unref(_this)->addConstraint(_unref(constraint));
	else
		_unref(_this)->addConstraint(_unref(constraint), disableCollisionsBetweenLinkedBodies->v.b);
}
DEFINE_PRIM(_VOID, btDiscreteDynamicsWorld_addConstraint, _IDL _IDL _NULL(_BOOL));

HL_PRIM void HL_NAME(btDiscreteDynamicsWorld_removeConstraint)(_ref(btDiscreteDynamicsWorld)* _this, _ref(btTypedConstraint)* constraint) {
	_unref(_this)->removeConstraint(_unref(constraint));
}
DEFINE_PRIM(_VOID, btDiscreteDynamicsWorld_removeConstraint, _IDL _IDL);

HL_PRIM int HL_NAME(btDiscreteDynamicsWorld_stepSimulation)(_ref(btDiscreteDynamicsWorld)* _this, float timeStep, vdynamic* maxSubSteps, vdynamic* fixedTimeStep) {
	if( !maxSubSteps )
		return _unref(_this)->stepSimulation(timeStep);
	else
	if( !fixedTimeStep )
		return _unref(_this)->stepSimulation(timeStep, maxSubSteps->v.i);
	else
		return _unref(_this)->stepSimulation(timeStep, maxSubSteps->v.i, fixedTimeStep->v.f);
}
DEFINE_PRIM(_I32, btDiscreteDynamicsWorld_stepSimulation, _IDL _F32 _NULL(_I32) _NULL(_F32));

HL_PRIM _ref(btRaycastVehicle::btVehicleTuning)* HL_NAME(btVehicleTuning_new0)() {
	return alloc_ref((new btRaycastVehicle::btVehicleTuning()),finalize_btVehicleTuning);
}
DEFINE_PRIM(_IDL, btVehicleTuning_new0,);

HL_PRIM float HL_NAME(btVehicleTuning_get_m_suspensionStiffness)( _ref(btRaycastVehicle::btVehicleTuning)* _this ) {
	return _unref(_this)->m_suspensionStiffness;
}
HL_PRIM void HL_NAME(btVehicleTuning_set_m_suspensionStiffness)( _ref(btRaycastVehicle::btVehicleTuning)* _this, float value ) {
	_unref(_this)->m_suspensionStiffness = (value);
}

HL_PRIM float HL_NAME(btVehicleTuning_get_m_suspensionCompression)( _ref(btRaycastVehicle::btVehicleTuning)* _this ) {
	return _unref(_this)->m_suspensionCompression;
}
HL_PRIM void HL_NAME(btVehicleTuning_set_m_suspensionCompression)( _ref(btRaycastVehicle::btVehicleTuning)* _this, float value ) {
	_unref(_this)->m_suspensionCompression = (value);
}

HL_PRIM float HL_NAME(btVehicleTuning_get_m_suspensionDamping)( _ref(btRaycastVehicle::btVehicleTuning)* _this ) {
	return _unref(_this)->m_suspensionDamping;
}
HL_PRIM void HL_NAME(btVehicleTuning_set_m_suspensionDamping)( _ref(btRaycastVehicle::btVehicleTuning)* _this, float value ) {
	_unref(_this)->m_suspensionDamping = (value);
}

HL_PRIM float HL_NAME(btVehicleTuning_get_m_maxSuspensionTravelCm)( _ref(btRaycastVehicle::btVehicleTuning)* _this ) {
	return _unref(_this)->m_maxSuspensionTravelCm;
}
HL_PRIM void HL_NAME(btVehicleTuning_set_m_maxSuspensionTravelCm)( _ref(btRaycastVehicle::btVehicleTuning)* _this, float value ) {
	_unref(_this)->m_maxSuspensionTravelCm = (value);
}

HL_PRIM float HL_NAME(btVehicleTuning_get_m_frictionSlip)( _ref(btRaycastVehicle::btVehicleTuning)* _this ) {
	return _unref(_this)->m_frictionSlip;
}
HL_PRIM void HL_NAME(btVehicleTuning_set_m_frictionSlip)( _ref(btRaycastVehicle::btVehicleTuning)* _this, float value ) {
	_unref(_this)->m_frictionSlip = (value);
}

HL_PRIM float HL_NAME(btVehicleTuning_get_m_maxSuspensionForce)( _ref(btRaycastVehicle::btVehicleTuning)* _this ) {
	return _unref(_this)->m_maxSuspensionForce;
}
HL_PRIM void HL_NAME(btVehicleTuning_set_m_maxSuspensionForce)( _ref(btRaycastVehicle::btVehicleTuning)* _this, float value ) {
	_unref(_this)->m_maxSuspensionForce = (value);
}

HL_PRIM _ref(btVector3)* HL_NAME(btVehicleRaycasterResult_get_m_hitPointInWorld)( _ref(btDefaultVehicleRaycaster::btVehicleRaycasterResult)* _this ) {
	return alloc_ref(new btVector3(_unref(_this)->m_hitPointInWorld),finalize_btVector3);
}
HL_PRIM void HL_NAME(btVehicleRaycasterResult_set_m_hitPointInWorld)( _ref(btDefaultVehicleRaycaster::btVehicleRaycasterResult)* _this, _ref(btVector3)* value ) {
	_unref(_this)->m_hitPointInWorld = *_unref(value);
}

HL_PRIM _ref(btVector3)* HL_NAME(btVehicleRaycasterResult_get_m_hitNormalInWorld)( _ref(btDefaultVehicleRaycaster::btVehicleRaycasterResult)* _this ) {
	return alloc_ref(new btVector3(_unref(_this)->m_hitNormalInWorld),finalize_btVector3);
}
HL_PRIM void HL_NAME(btVehicleRaycasterResult_set_m_hitNormalInWorld)( _ref(btDefaultVehicleRaycaster::btVehicleRaycasterResult)* _this, _ref(btVector3)* value ) {
	_unref(_this)->m_hitNormalInWorld = *_unref(value);
}

HL_PRIM float HL_NAME(btVehicleRaycasterResult_get_m_distFraction)( _ref(btDefaultVehicleRaycaster::btVehicleRaycasterResult)* _this ) {
	return _unref(_this)->m_distFraction;
}
HL_PRIM void HL_NAME(btVehicleRaycasterResult_set_m_distFraction)( _ref(btDefaultVehicleRaycaster::btVehicleRaycasterResult)* _this, float value ) {
	_unref(_this)->m_distFraction = (value);
}

HL_PRIM void HL_NAME(btVehicleRaycaster_castRay)(_ref(btVehicleRaycaster)* _this, _ref(btVector3)* from, _ref(btVector3)* to, _ref(btDefaultVehicleRaycaster::btVehicleRaycasterResult)* result) {
	_unref(_this)->castRay(*_unref(from), *_unref(to), *_unref(result));
}
DEFINE_PRIM(_VOID, btVehicleRaycaster_castRay, _IDL _IDL _IDL _IDL);

HL_PRIM _ref(btDefaultVehicleRaycaster)* HL_NAME(btDefaultVehicleRaycaster_new1)(_ref(btDynamicsWorld)* world) {
	return alloc_ref((new btDefaultVehicleRaycaster(_unref(world))),finalize_btDefaultVehicleRaycaster);
}
DEFINE_PRIM(_IDL, btDefaultVehicleRaycaster_new1, _IDL);

HL_PRIM _ref(btVector3)* HL_NAME(RaycastInfo_get_m_contactNormalWS)( _ref(btWheelInfo::RaycastInfo)* _this ) {
	return alloc_ref(new btVector3(_unref(_this)->m_contactNormalWS),finalize_btVector3);
}
HL_PRIM void HL_NAME(RaycastInfo_set_m_contactNormalWS)( _ref(btWheelInfo::RaycastInfo)* _this, _ref(btVector3)* value ) {
	_unref(_this)->m_contactNormalWS = *_unref(value);
}

HL_PRIM _ref(btVector3)* HL_NAME(RaycastInfo_get_m_contactPointWS)( _ref(btWheelInfo::RaycastInfo)* _this ) {
	return alloc_ref(new btVector3(_unref(_this)->m_contactPointWS),finalize_btVector3);
}
HL_PRIM void HL_NAME(RaycastInfo_set_m_contactPointWS)( _ref(btWheelInfo::RaycastInfo)* _this, _ref(btVector3)* value ) {
	_unref(_this)->m_contactPointWS = *_unref(value);
}

HL_PRIM float HL_NAME(RaycastInfo_get_m_suspensionLength)( _ref(btWheelInfo::RaycastInfo)* _this ) {
	return _unref(_this)->m_suspensionLength;
}
HL_PRIM void HL_NAME(RaycastInfo_set_m_suspensionLength)( _ref(btWheelInfo::RaycastInfo)* _this, float value ) {
	_unref(_this)->m_suspensionLength = (value);
}

HL_PRIM _ref(btVector3)* HL_NAME(RaycastInfo_get_m_hardPointWS)( _ref(btWheelInfo::RaycastInfo)* _this ) {
	return alloc_ref(new btVector3(_unref(_this)->m_hardPointWS),finalize_btVector3);
}
HL_PRIM void HL_NAME(RaycastInfo_set_m_hardPointWS)( _ref(btWheelInfo::RaycastInfo)* _this, _ref(btVector3)* value ) {
	_unref(_this)->m_hardPointWS = *_unref(value);
}

HL_PRIM _ref(btVector3)* HL_NAME(RaycastInfo_get_m_wheelDirectionWS)( _ref(btWheelInfo::RaycastInfo)* _this ) {
	return alloc_ref(new btVector3(_unref(_this)->m_wheelDirectionWS),finalize_btVector3);
}
HL_PRIM void HL_NAME(RaycastInfo_set_m_wheelDirectionWS)( _ref(btWheelInfo::RaycastInfo)* _this, _ref(btVector3)* value ) {
	_unref(_this)->m_wheelDirectionWS = *_unref(value);
}

HL_PRIM _ref(btVector3)* HL_NAME(RaycastInfo_get_m_wheelAxleWS)( _ref(btWheelInfo::RaycastInfo)* _this ) {
	return alloc_ref(new btVector3(_unref(_this)->m_wheelAxleWS),finalize_btVector3);
}
HL_PRIM void HL_NAME(RaycastInfo_set_m_wheelAxleWS)( _ref(btWheelInfo::RaycastInfo)* _this, _ref(btVector3)* value ) {
	_unref(_this)->m_wheelAxleWS = *_unref(value);
}

HL_PRIM bool HL_NAME(RaycastInfo_get_m_isInContact)( _ref(btWheelInfo::RaycastInfo)* _this ) {
	return _unref(_this)->m_isInContact;
}
HL_PRIM void HL_NAME(RaycastInfo_set_m_isInContact)( _ref(btWheelInfo::RaycastInfo)* _this, bool value ) {
	_unref(_this)->m_isInContact = (value);
}

HL_PRIM void* HL_NAME(RaycastInfo_get_m_groundObject)( _ref(btWheelInfo::RaycastInfo)* _this ) {
	return _unref(_this)->m_groundObject;
}
HL_PRIM void HL_NAME(RaycastInfo_set_m_groundObject)( _ref(btWheelInfo::RaycastInfo)* _this, void* value ) {
	_unref(_this)->m_groundObject = (value);
}

HL_PRIM _ref(btVector3)* HL_NAME(btWheelInfoConstructionInfo_get_m_chassisConnectionCS)( _ref(btWheelInfoConstructionInfo)* _this ) {
	return alloc_ref(new btVector3(_unref(_this)->m_chassisConnectionCS),finalize_btVector3);
}
HL_PRIM void HL_NAME(btWheelInfoConstructionInfo_set_m_chassisConnectionCS)( _ref(btWheelInfoConstructionInfo)* _this, _ref(btVector3)* value ) {
	_unref(_this)->m_chassisConnectionCS = *_unref(value);
}

HL_PRIM _ref(btVector3)* HL_NAME(btWheelInfoConstructionInfo_get_m_wheelDirectionCS)( _ref(btWheelInfoConstructionInfo)* _this ) {
	return alloc_ref(new btVector3(_unref(_this)->m_wheelDirectionCS),finalize_btVector3);
}
HL_PRIM void HL_NAME(btWheelInfoConstructionInfo_set_m_wheelDirectionCS)( _ref(btWheelInfoConstructionInfo)* _this, _ref(btVector3)* value ) {
	_unref(_this)->m_wheelDirectionCS = *_unref(value);
}

HL_PRIM _ref(btVector3)* HL_NAME(btWheelInfoConstructionInfo_get_m_wheelAxleCS)( _ref(btWheelInfoConstructionInfo)* _this ) {
	return alloc_ref(new btVector3(_unref(_this)->m_wheelAxleCS),finalize_btVector3);
}
HL_PRIM void HL_NAME(btWheelInfoConstructionInfo_set_m_wheelAxleCS)( _ref(btWheelInfoConstructionInfo)* _this, _ref(btVector3)* value ) {
	_unref(_this)->m_wheelAxleCS = *_unref(value);
}

HL_PRIM float HL_NAME(btWheelInfoConstructionInfo_get_m_suspensionRestLength)( _ref(btWheelInfoConstructionInfo)* _this ) {
	return _unref(_this)->m_suspensionRestLength;
}
HL_PRIM void HL_NAME(btWheelInfoConstructionInfo_set_m_suspensionRestLength)( _ref(btWheelInfoConstructionInfo)* _this, float value ) {
	_unref(_this)->m_suspensionRestLength = (value);
}

HL_PRIM float HL_NAME(btWheelInfoConstructionInfo_get_m_maxSuspensionTravelCm)( _ref(btWheelInfoConstructionInfo)* _this ) {
	return _unref(_this)->m_maxSuspensionTravelCm;
}
HL_PRIM void HL_NAME(btWheelInfoConstructionInfo_set_m_maxSuspensionTravelCm)( _ref(btWheelInfoConstructionInfo)* _this, float value ) {
	_unref(_this)->m_maxSuspensionTravelCm = (value);
}

HL_PRIM float HL_NAME(btWheelInfoConstructionInfo_get_m_wheelRadius)( _ref(btWheelInfoConstructionInfo)* _this ) {
	return _unref(_this)->m_wheelRadius;
}
HL_PRIM void HL_NAME(btWheelInfoConstructionInfo_set_m_wheelRadius)( _ref(btWheelInfoConstructionInfo)* _this, float value ) {
	_unref(_this)->m_wheelRadius = (value);
}

HL_PRIM float HL_NAME(btWheelInfoConstructionInfo_get_m_suspensionStiffness)( _ref(btWheelInfoConstructionInfo)* _this ) {
	return _unref(_this)->m_suspensionStiffness;
}
HL_PRIM void HL_NAME(btWheelInfoConstructionInfo_set_m_suspensionStiffness)( _ref(btWheelInfoConstructionInfo)* _this, float value ) {
	_unref(_this)->m_suspensionStiffness = (value);
}

HL_PRIM float HL_NAME(btWheelInfoConstructionInfo_get_m_wheelsDampingCompression)( _ref(btWheelInfoConstructionInfo)* _this ) {
	return _unref(_this)->m_wheelsDampingCompression;
}
HL_PRIM void HL_NAME(btWheelInfoConstructionInfo_set_m_wheelsDampingCompression)( _ref(btWheelInfoConstructionInfo)* _this, float value ) {
	_unref(_this)->m_wheelsDampingCompression = (value);
}

HL_PRIM float HL_NAME(btWheelInfoConstructionInfo_get_m_wheelsDampingRelaxation)( _ref(btWheelInfoConstructionInfo)* _this ) {
	return _unref(_this)->m_wheelsDampingRelaxation;
}
HL_PRIM void HL_NAME(btWheelInfoConstructionInfo_set_m_wheelsDampingRelaxation)( _ref(btWheelInfoConstructionInfo)* _this, float value ) {
	_unref(_this)->m_wheelsDampingRelaxation = (value);
}

HL_PRIM float HL_NAME(btWheelInfoConstructionInfo_get_m_frictionSlip)( _ref(btWheelInfoConstructionInfo)* _this ) {
	return _unref(_this)->m_frictionSlip;
}
HL_PRIM void HL_NAME(btWheelInfoConstructionInfo_set_m_frictionSlip)( _ref(btWheelInfoConstructionInfo)* _this, float value ) {
	_unref(_this)->m_frictionSlip = (value);
}

HL_PRIM float HL_NAME(btWheelInfoConstructionInfo_get_m_maxSuspensionForce)( _ref(btWheelInfoConstructionInfo)* _this ) {
	return _unref(_this)->m_maxSuspensionForce;
}
HL_PRIM void HL_NAME(btWheelInfoConstructionInfo_set_m_maxSuspensionForce)( _ref(btWheelInfoConstructionInfo)* _this, float value ) {
	_unref(_this)->m_maxSuspensionForce = (value);
}

HL_PRIM bool HL_NAME(btWheelInfoConstructionInfo_get_m_bIsFrontWheel)( _ref(btWheelInfoConstructionInfo)* _this ) {
	return _unref(_this)->m_bIsFrontWheel;
}
HL_PRIM void HL_NAME(btWheelInfoConstructionInfo_set_m_bIsFrontWheel)( _ref(btWheelInfoConstructionInfo)* _this, bool value ) {
	_unref(_this)->m_bIsFrontWheel = (value);
}

HL_PRIM float HL_NAME(btWheelInfo_get_m_suspensionStiffness)( _ref(btWheelInfo)* _this ) {
	return _unref(_this)->m_suspensionStiffness;
}
HL_PRIM void HL_NAME(btWheelInfo_set_m_suspensionStiffness)( _ref(btWheelInfo)* _this, float value ) {
	_unref(_this)->m_suspensionStiffness = (value);
}

HL_PRIM float HL_NAME(btWheelInfo_get_m_frictionSlip)( _ref(btWheelInfo)* _this ) {
	return _unref(_this)->m_frictionSlip;
}
HL_PRIM void HL_NAME(btWheelInfo_set_m_frictionSlip)( _ref(btWheelInfo)* _this, float value ) {
	_unref(_this)->m_frictionSlip = (value);
}

HL_PRIM float HL_NAME(btWheelInfo_get_m_engineForce)( _ref(btWheelInfo)* _this ) {
	return _unref(_this)->m_engineForce;
}
HL_PRIM void HL_NAME(btWheelInfo_set_m_engineForce)( _ref(btWheelInfo)* _this, float value ) {
	_unref(_this)->m_engineForce = (value);
}

HL_PRIM float HL_NAME(btWheelInfo_get_m_rollInfluence)( _ref(btWheelInfo)* _this ) {
	return _unref(_this)->m_rollInfluence;
}
HL_PRIM void HL_NAME(btWheelInfo_set_m_rollInfluence)( _ref(btWheelInfo)* _this, float value ) {
	_unref(_this)->m_rollInfluence = (value);
}

HL_PRIM float HL_NAME(btWheelInfo_get_m_suspensionRestLength1)( _ref(btWheelInfo)* _this ) {
	return _unref(_this)->m_suspensionRestLength1;
}
HL_PRIM void HL_NAME(btWheelInfo_set_m_suspensionRestLength1)( _ref(btWheelInfo)* _this, float value ) {
	_unref(_this)->m_suspensionRestLength1 = (value);
}

HL_PRIM float HL_NAME(btWheelInfo_get_m_wheelsRadius)( _ref(btWheelInfo)* _this ) {
	return _unref(_this)->m_wheelsRadius;
}
HL_PRIM void HL_NAME(btWheelInfo_set_m_wheelsRadius)( _ref(btWheelInfo)* _this, float value ) {
	_unref(_this)->m_wheelsRadius = (value);
}

HL_PRIM float HL_NAME(btWheelInfo_get_m_wheelsDampingCompression)( _ref(btWheelInfo)* _this ) {
	return _unref(_this)->m_wheelsDampingCompression;
}
HL_PRIM void HL_NAME(btWheelInfo_set_m_wheelsDampingCompression)( _ref(btWheelInfo)* _this, float value ) {
	_unref(_this)->m_wheelsDampingCompression = (value);
}

HL_PRIM float HL_NAME(btWheelInfo_get_m_wheelsDampingRelaxation)( _ref(btWheelInfo)* _this ) {
	return _unref(_this)->m_wheelsDampingRelaxation;
}
HL_PRIM void HL_NAME(btWheelInfo_set_m_wheelsDampingRelaxation)( _ref(btWheelInfo)* _this, float value ) {
	_unref(_this)->m_wheelsDampingRelaxation = (value);
}

HL_PRIM float HL_NAME(btWheelInfo_get_m_steering)( _ref(btWheelInfo)* _this ) {
	return _unref(_this)->m_steering;
}
HL_PRIM void HL_NAME(btWheelInfo_set_m_steering)( _ref(btWheelInfo)* _this, float value ) {
	_unref(_this)->m_steering = (value);
}

HL_PRIM float HL_NAME(btWheelInfo_get_m_maxSuspensionForce)( _ref(btWheelInfo)* _this ) {
	return _unref(_this)->m_maxSuspensionForce;
}
HL_PRIM void HL_NAME(btWheelInfo_set_m_maxSuspensionForce)( _ref(btWheelInfo)* _this, float value ) {
	_unref(_this)->m_maxSuspensionForce = (value);
}

HL_PRIM float HL_NAME(btWheelInfo_get_m_maxSuspensionTravelCm)( _ref(btWheelInfo)* _this ) {
	return _unref(_this)->m_maxSuspensionTravelCm;
}
HL_PRIM void HL_NAME(btWheelInfo_set_m_maxSuspensionTravelCm)( _ref(btWheelInfo)* _this, float value ) {
	_unref(_this)->m_maxSuspensionTravelCm = (value);
}

HL_PRIM float HL_NAME(btWheelInfo_get_m_wheelsSuspensionForce)( _ref(btWheelInfo)* _this ) {
	return _unref(_this)->m_wheelsSuspensionForce;
}
HL_PRIM void HL_NAME(btWheelInfo_set_m_wheelsSuspensionForce)( _ref(btWheelInfo)* _this, float value ) {
	_unref(_this)->m_wheelsSuspensionForce = (value);
}

HL_PRIM bool HL_NAME(btWheelInfo_get_m_bIsFrontWheel)( _ref(btWheelInfo)* _this ) {
	return _unref(_this)->m_bIsFrontWheel;
}
HL_PRIM void HL_NAME(btWheelInfo_set_m_bIsFrontWheel)( _ref(btWheelInfo)* _this, bool value ) {
	_unref(_this)->m_bIsFrontWheel = (value);
}

HL_PRIM _ref(btWheelInfo::RaycastInfo)* HL_NAME(btWheelInfo_get_m_raycastInfo)( _ref(btWheelInfo)* _this ) {
	return alloc_ref(new btWheelInfo::RaycastInfo(_unref(_this)->m_raycastInfo),finalize_RaycastInfo);
}
HL_PRIM void HL_NAME(btWheelInfo_set_m_raycastInfo)( _ref(btWheelInfo)* _this, _ref(btWheelInfo::RaycastInfo)* value ) {
	_unref(_this)->m_raycastInfo = *_unref(value);
}

HL_PRIM _ref(btVector3)* HL_NAME(btWheelInfo_get_m_chassisConnectionPointCS)( _ref(btWheelInfo)* _this ) {
	return alloc_ref(new btVector3(_unref(_this)->m_chassisConnectionPointCS),finalize_btVector3);
}
HL_PRIM void HL_NAME(btWheelInfo_set_m_chassisConnectionPointCS)( _ref(btWheelInfo)* _this, _ref(btVector3)* value ) {
	_unref(_this)->m_chassisConnectionPointCS = *_unref(value);
}

HL_PRIM _ref(btWheelInfo)* HL_NAME(btWheelInfo_new1)(_ref(btWheelInfoConstructionInfo)* ci) {
	return alloc_ref((new btWheelInfo(*_unref(ci))),finalize_btWheelInfo);
}
DEFINE_PRIM(_IDL, btWheelInfo_new1, _IDL);

HL_PRIM float HL_NAME(btWheelInfo_getSuspensionRestLength)(_ref(btWheelInfo)* _this) {
	return _unref(_this)->getSuspensionRestLength();
}
DEFINE_PRIM(_F32, btWheelInfo_getSuspensionRestLength, _IDL);

HL_PRIM void HL_NAME(btWheelInfo_updateWheel)(_ref(btWheelInfo)* _this, _ref(btRigidBody)* chassis, _ref(btWheelInfo::RaycastInfo)* raycastInfo) {
	_unref(_this)->updateWheel(*_unref(chassis), *_unref(raycastInfo));
}
DEFINE_PRIM(_VOID, btWheelInfo_updateWheel, _IDL _IDL _IDL);

HL_PRIM _ref(btTransform)* HL_NAME(btWheelInfo_get_m_worldTransform)( _ref(btWheelInfo)* _this ) {
	return alloc_ref(new btTransform(_unref(_this)->m_worldTransform),finalize_btTransform);
}
HL_PRIM void HL_NAME(btWheelInfo_set_m_worldTransform)( _ref(btWheelInfo)* _this, _ref(btTransform)* value ) {
	_unref(_this)->m_worldTransform = *_unref(value);
}

HL_PRIM _ref(btVector3)* HL_NAME(btWheelInfo_get_m_wheelDirectionCS)( _ref(btWheelInfo)* _this ) {
	return alloc_ref(new btVector3(_unref(_this)->m_wheelDirectionCS),finalize_btVector3);
}
HL_PRIM void HL_NAME(btWheelInfo_set_m_wheelDirectionCS)( _ref(btWheelInfo)* _this, _ref(btVector3)* value ) {
	_unref(_this)->m_wheelDirectionCS = *_unref(value);
}

HL_PRIM _ref(btVector3)* HL_NAME(btWheelInfo_get_m_wheelAxleCS)( _ref(btWheelInfo)* _this ) {
	return alloc_ref(new btVector3(_unref(_this)->m_wheelAxleCS),finalize_btVector3);
}
HL_PRIM void HL_NAME(btWheelInfo_set_m_wheelAxleCS)( _ref(btWheelInfo)* _this, _ref(btVector3)* value ) {
	_unref(_this)->m_wheelAxleCS = *_unref(value);
}

HL_PRIM float HL_NAME(btWheelInfo_get_m_rotation)( _ref(btWheelInfo)* _this ) {
	return _unref(_this)->m_rotation;
}
HL_PRIM void HL_NAME(btWheelInfo_set_m_rotation)( _ref(btWheelInfo)* _this, float value ) {
	_unref(_this)->m_rotation = (value);
}

HL_PRIM float HL_NAME(btWheelInfo_get_m_deltaRotation)( _ref(btWheelInfo)* _this ) {
	return _unref(_this)->m_deltaRotation;
}
HL_PRIM void HL_NAME(btWheelInfo_set_m_deltaRotation)( _ref(btWheelInfo)* _this, float value ) {
	_unref(_this)->m_deltaRotation = (value);
}

HL_PRIM float HL_NAME(btWheelInfo_get_m_brake)( _ref(btWheelInfo)* _this ) {
	return _unref(_this)->m_brake;
}
HL_PRIM void HL_NAME(btWheelInfo_set_m_brake)( _ref(btWheelInfo)* _this, float value ) {
	_unref(_this)->m_brake = (value);
}

HL_PRIM float HL_NAME(btWheelInfo_get_m_clippedInvContactDotSuspension)( _ref(btWheelInfo)* _this ) {
	return _unref(_this)->m_clippedInvContactDotSuspension;
}
HL_PRIM void HL_NAME(btWheelInfo_set_m_clippedInvContactDotSuspension)( _ref(btWheelInfo)* _this, float value ) {
	_unref(_this)->m_clippedInvContactDotSuspension = (value);
}

HL_PRIM float HL_NAME(btWheelInfo_get_m_suspensionRelativeVelocity)( _ref(btWheelInfo)* _this ) {
	return _unref(_this)->m_suspensionRelativeVelocity;
}
HL_PRIM void HL_NAME(btWheelInfo_set_m_suspensionRelativeVelocity)( _ref(btWheelInfo)* _this, float value ) {
	_unref(_this)->m_suspensionRelativeVelocity = (value);
}

HL_PRIM float HL_NAME(btWheelInfo_get_m_skidInfo)( _ref(btWheelInfo)* _this ) {
	return _unref(_this)->m_skidInfo;
}
HL_PRIM void HL_NAME(btWheelInfo_set_m_skidInfo)( _ref(btWheelInfo)* _this, float value ) {
	_unref(_this)->m_skidInfo = (value);
}

HL_PRIM void HL_NAME(btActionInterface_updateAction)(_ref(btActionInterface)* _this, _ref(btCollisionWorld)* collisionWorld, float deltaTimeStep) {
	_unref(_this)->updateAction(_unref(collisionWorld), deltaTimeStep);
}
DEFINE_PRIM(_VOID, btActionInterface_updateAction, _IDL _IDL _F32);

HL_PRIM _ref(btKinematicCharacterController)* HL_NAME(btKinematicCharacterController_new4)(_ref(btPairCachingGhostObject)* ghostObject, _ref(btConvexShape)* convexShape, float stepHeight, _ref(btVector3)* upAxis) {
	if( !upAxis )
		return alloc_ref((new btKinematicCharacterController(_unref(ghostObject), _unref(convexShape), stepHeight)),finalize_btKinematicCharacterController);
	else
		return alloc_ref((new btKinematicCharacterController(_unref(ghostObject), _unref(convexShape), stepHeight, *_unref(upAxis))),finalize_btKinematicCharacterController);
}
DEFINE_PRIM(_IDL, btKinematicCharacterController_new4, _IDL _IDL _F32 _IDL);

HL_PRIM void HL_NAME(btKinematicCharacterController_setUp)(_ref(btKinematicCharacterController)* _this, _ref(btVector3)* axis) {
	_unref(_this)->setUp(*_unref(axis));
}
DEFINE_PRIM(_VOID, btKinematicCharacterController_setUp, _IDL _IDL);

HL_PRIM void HL_NAME(btKinematicCharacterController_setWalkDirection)(_ref(btKinematicCharacterController)* _this, _ref(btVector3)* walkDirection) {
	_unref(_this)->setWalkDirection(*_unref(walkDirection));
}
DEFINE_PRIM(_VOID, btKinematicCharacterController_setWalkDirection, _IDL _IDL);

HL_PRIM void HL_NAME(btKinematicCharacterController_setVelocityForTimeInterval)(_ref(btKinematicCharacterController)* _this, _ref(btVector3)* velocity, float timeInterval) {
	_unref(_this)->setVelocityForTimeInterval(*_unref(velocity), timeInterval);
}
DEFINE_PRIM(_VOID, btKinematicCharacterController_setVelocityForTimeInterval, _IDL _IDL _F32);

HL_PRIM void HL_NAME(btKinematicCharacterController_warp)(_ref(btKinematicCharacterController)* _this, _ref(btVector3)* origin) {
	_unref(_this)->warp(*_unref(origin));
}
DEFINE_PRIM(_VOID, btKinematicCharacterController_warp, _IDL _IDL);

HL_PRIM void HL_NAME(btKinematicCharacterController_preStep)(_ref(btKinematicCharacterController)* _this, _ref(btCollisionWorld)* collisionWorld) {
	_unref(_this)->preStep(_unref(collisionWorld));
}
DEFINE_PRIM(_VOID, btKinematicCharacterController_preStep, _IDL _IDL);

HL_PRIM void HL_NAME(btKinematicCharacterController_playerStep)(_ref(btKinematicCharacterController)* _this, _ref(btCollisionWorld)* collisionWorld, float dt) {
	_unref(_this)->playerStep(_unref(collisionWorld), dt);
}
DEFINE_PRIM(_VOID, btKinematicCharacterController_playerStep, _IDL _IDL _F32);

HL_PRIM void HL_NAME(btKinematicCharacterController_setFallSpeed)(_ref(btKinematicCharacterController)* _this, float fallSpeed) {
	_unref(_this)->setFallSpeed(fallSpeed);
}
DEFINE_PRIM(_VOID, btKinematicCharacterController_setFallSpeed, _IDL _F32);

HL_PRIM void HL_NAME(btKinematicCharacterController_setJumpSpeed)(_ref(btKinematicCharacterController)* _this, float jumpSpeed) {
	_unref(_this)->setJumpSpeed(jumpSpeed);
}
DEFINE_PRIM(_VOID, btKinematicCharacterController_setJumpSpeed, _IDL _F32);

HL_PRIM void HL_NAME(btKinematicCharacterController_setMaxJumpHeight)(_ref(btKinematicCharacterController)* _this, float maxJumpHeight) {
	_unref(_this)->setMaxJumpHeight(maxJumpHeight);
}
DEFINE_PRIM(_VOID, btKinematicCharacterController_setMaxJumpHeight, _IDL _F32);

HL_PRIM bool HL_NAME(btKinematicCharacterController_canJump)(_ref(btKinematicCharacterController)* _this) {
	return _unref(_this)->canJump();
}
DEFINE_PRIM(_BOOL, btKinematicCharacterController_canJump, _IDL);

HL_PRIM void HL_NAME(btKinematicCharacterController_jump)(_ref(btKinematicCharacterController)* _this) {
	_unref(_this)->jump();
}
DEFINE_PRIM(_VOID, btKinematicCharacterController_jump, _IDL);

HL_PRIM void HL_NAME(btKinematicCharacterController_setGravity)(_ref(btKinematicCharacterController)* _this, _ref(btVector3)* gravity) {
	_unref(_this)->setGravity(*_unref(gravity));
}
DEFINE_PRIM(_VOID, btKinematicCharacterController_setGravity, _IDL _IDL);

HL_PRIM _ref(btVector3)* HL_NAME(btKinematicCharacterController_getGravity)(_ref(btKinematicCharacterController)* _this) {
	return alloc_ref(new btVector3(_unref(_this)->getGravity()),finalize_btVector3);
}
DEFINE_PRIM(_IDL, btKinematicCharacterController_getGravity, _IDL);

HL_PRIM void HL_NAME(btKinematicCharacterController_setMaxSlope)(_ref(btKinematicCharacterController)* _this, float slopeRadians) {
	_unref(_this)->setMaxSlope(slopeRadians);
}
DEFINE_PRIM(_VOID, btKinematicCharacterController_setMaxSlope, _IDL _F32);

HL_PRIM float HL_NAME(btKinematicCharacterController_getMaxSlope)(_ref(btKinematicCharacterController)* _this) {
	return _unref(_this)->getMaxSlope();
}
DEFINE_PRIM(_F32, btKinematicCharacterController_getMaxSlope, _IDL);

HL_PRIM _ref(btPairCachingGhostObject)* HL_NAME(btKinematicCharacterController_getGhostObject)(_ref(btKinematicCharacterController)* _this) {
	return alloc_ref((_unref(_this)->getGhostObject()),finalize_btPairCachingGhostObject);
}
DEFINE_PRIM(_IDL, btKinematicCharacterController_getGhostObject, _IDL);

HL_PRIM void HL_NAME(btKinematicCharacterController_setUseGhostSweepTest)(_ref(btKinematicCharacterController)* _this, bool useGhostObjectSweepTest) {
	_unref(_this)->setUseGhostSweepTest(useGhostObjectSweepTest);
}
DEFINE_PRIM(_VOID, btKinematicCharacterController_setUseGhostSweepTest, _IDL _BOOL);

HL_PRIM bool HL_NAME(btKinematicCharacterController_onGround)(_ref(btKinematicCharacterController)* _this) {
	return _unref(_this)->onGround();
}
DEFINE_PRIM(_BOOL, btKinematicCharacterController_onGround, _IDL);

HL_PRIM _ref(btRaycastVehicle)* HL_NAME(btRaycastVehicle_new3)(_ref(btRaycastVehicle::btVehicleTuning)* tuning, _ref(btRigidBody)* chassis, _ref(btVehicleRaycaster)* raycaster) {
	return alloc_ref((new btRaycastVehicle(*_unref(tuning), _unref(chassis), _unref(raycaster))),finalize_btRaycastVehicle);
}
DEFINE_PRIM(_IDL, btRaycastVehicle_new3, _IDL _IDL _IDL);

HL_PRIM void HL_NAME(btRaycastVehicle_applyEngineForce)(_ref(btRaycastVehicle)* _this, float force, int wheel) {
	_unref(_this)->applyEngineForce(force, wheel);
}
DEFINE_PRIM(_VOID, btRaycastVehicle_applyEngineForce, _IDL _F32 _I32);

HL_PRIM void HL_NAME(btRaycastVehicle_setSteeringValue)(_ref(btRaycastVehicle)* _this, float steering, int wheel) {
	_unref(_this)->setSteeringValue(steering, wheel);
}
DEFINE_PRIM(_VOID, btRaycastVehicle_setSteeringValue, _IDL _F32 _I32);

HL_PRIM _ref(btTransform)* HL_NAME(btRaycastVehicle_getWheelTransformWS)(_ref(btRaycastVehicle)* _this, int wheelIndex) {
	return alloc_ref(new btTransform(_unref(_this)->getWheelTransformWS(wheelIndex)),finalize_btTransform);
}
DEFINE_PRIM(_IDL, btRaycastVehicle_getWheelTransformWS, _IDL _I32);

HL_PRIM void HL_NAME(btRaycastVehicle_updateWheelTransform)(_ref(btRaycastVehicle)* _this, int wheelIndex, bool interpolatedTransform) {
	_unref(_this)->updateWheelTransform(wheelIndex, interpolatedTransform);
}
DEFINE_PRIM(_VOID, btRaycastVehicle_updateWheelTransform, _IDL _I32 _BOOL);

HL_PRIM _ref(btWheelInfo)* HL_NAME(btRaycastVehicle_addWheel)(_ref(btRaycastVehicle)* _this, _ref(btVector3)* connectionPointCS0, _ref(btVector3)* wheelDirectionCS0, _ref(btVector3)* wheelAxleCS, float suspensionRestLength, float wheelRadius, _ref(btRaycastVehicle::btVehicleTuning)* tuning, bool isFrontWheel) {
	return alloc_ref(new btWheelInfo(_unref(_this)->addWheel(*_unref(connectionPointCS0), *_unref(wheelDirectionCS0), *_unref(wheelAxleCS), suspensionRestLength, wheelRadius, *_unref(tuning), isFrontWheel)),finalize_btWheelInfo);
}
DEFINE_PRIM(_IDL, btRaycastVehicle_addWheel, _IDL _IDL _IDL _IDL _F32 _F32 _IDL _BOOL);

HL_PRIM int HL_NAME(btRaycastVehicle_getNumWheels)(_ref(btRaycastVehicle)* _this) {
	return _unref(_this)->getNumWheels();
}
DEFINE_PRIM(_I32, btRaycastVehicle_getNumWheels, _IDL);

HL_PRIM _ref(btRigidBody)* HL_NAME(btRaycastVehicle_getRigidBody)(_ref(btRaycastVehicle)* _this) {
	return alloc_ref((_unref(_this)->getRigidBody()),finalize_btRigidBody);
}
DEFINE_PRIM(_IDL, btRaycastVehicle_getRigidBody, _IDL);

HL_PRIM _ref(btWheelInfo)* HL_NAME(btRaycastVehicle_getWheelInfo)(_ref(btRaycastVehicle)* _this, int index) {
	return alloc_ref(new btWheelInfo(_unref(_this)->getWheelInfo(index)),finalize_btWheelInfo);
}
DEFINE_PRIM(_IDL, btRaycastVehicle_getWheelInfo, _IDL _I32);

HL_PRIM void HL_NAME(btRaycastVehicle_setBrake)(_ref(btRaycastVehicle)* _this, float brake, int wheelIndex) {
	_unref(_this)->setBrake(brake, wheelIndex);
}
DEFINE_PRIM(_VOID, btRaycastVehicle_setBrake, _IDL _F32 _I32);

HL_PRIM void HL_NAME(btRaycastVehicle_setCoordinateSystem)(_ref(btRaycastVehicle)* _this, int rightIndex, int upIndex, int forwardIndex) {
	_unref(_this)->setCoordinateSystem(rightIndex, upIndex, forwardIndex);
}
DEFINE_PRIM(_VOID, btRaycastVehicle_setCoordinateSystem, _IDL _I32 _I32 _I32);

HL_PRIM float HL_NAME(btRaycastVehicle_getCurrentSpeedKmHour)(_ref(btRaycastVehicle)* _this) {
	return _unref(_this)->getCurrentSpeedKmHour();
}
DEFINE_PRIM(_F32, btRaycastVehicle_getCurrentSpeedKmHour, _IDL);

HL_PRIM _ref(btTransform)* HL_NAME(btRaycastVehicle_getChassisWorldTransform)(_ref(btRaycastVehicle)* _this) {
	return alloc_ref(new btTransform(_unref(_this)->getChassisWorldTransform()),finalize_btTransform);
}
DEFINE_PRIM(_IDL, btRaycastVehicle_getChassisWorldTransform, _IDL);

HL_PRIM float HL_NAME(btRaycastVehicle_rayCast)(_ref(btRaycastVehicle)* _this, _ref(btWheelInfo)* wheel) {
	return _unref(_this)->rayCast(*_unref(wheel));
}
DEFINE_PRIM(_F32, btRaycastVehicle_rayCast, _IDL _IDL);

HL_PRIM void HL_NAME(btRaycastVehicle_updateVehicle)(_ref(btRaycastVehicle)* _this, float step) {
	_unref(_this)->updateVehicle(step);
}
DEFINE_PRIM(_VOID, btRaycastVehicle_updateVehicle, _IDL _F32);

HL_PRIM void HL_NAME(btRaycastVehicle_resetSuspension)(_ref(btRaycastVehicle)* _this) {
	_unref(_this)->resetSuspension();
}
DEFINE_PRIM(_VOID, btRaycastVehicle_resetSuspension, _IDL);

HL_PRIM float HL_NAME(btRaycastVehicle_getSteeringValue)(_ref(btRaycastVehicle)* _this, int wheel) {
	return _unref(_this)->getSteeringValue(wheel);
}
DEFINE_PRIM(_F32, btRaycastVehicle_getSteeringValue, _IDL _I32);

HL_PRIM void HL_NAME(btRaycastVehicle_updateWheelTransformsWS)(_ref(btRaycastVehicle)* _this, _ref(btWheelInfo)* wheel, vdynamic* interpolatedTransform) {
	if( !interpolatedTransform )
		_unref(_this)->updateWheelTransformsWS(*_unref(wheel));
	else
		_unref(_this)->updateWheelTransformsWS(*_unref(wheel), interpolatedTransform->v.b);
}
DEFINE_PRIM(_VOID, btRaycastVehicle_updateWheelTransformsWS, _IDL _IDL _NULL(_BOOL));

HL_PRIM void HL_NAME(btRaycastVehicle_setPitchControl)(_ref(btRaycastVehicle)* _this, float pitch) {
	_unref(_this)->setPitchControl(pitch);
}
DEFINE_PRIM(_VOID, btRaycastVehicle_setPitchControl, _IDL _F32);

HL_PRIM void HL_NAME(btRaycastVehicle_updateSuspension)(_ref(btRaycastVehicle)* _this, float deltaTime) {
	_unref(_this)->updateSuspension(deltaTime);
}
DEFINE_PRIM(_VOID, btRaycastVehicle_updateSuspension, _IDL _F32);

HL_PRIM void HL_NAME(btRaycastVehicle_updateFriction)(_ref(btRaycastVehicle)* _this, float timeStep) {
	_unref(_this)->updateFriction(timeStep);
}
DEFINE_PRIM(_VOID, btRaycastVehicle_updateFriction, _IDL _F32);

HL_PRIM int HL_NAME(btRaycastVehicle_getRightAxis)(_ref(btRaycastVehicle)* _this) {
	return _unref(_this)->getRightAxis();
}
DEFINE_PRIM(_I32, btRaycastVehicle_getRightAxis, _IDL);

HL_PRIM int HL_NAME(btRaycastVehicle_getUpAxis)(_ref(btRaycastVehicle)* _this) {
	return _unref(_this)->getUpAxis();
}
DEFINE_PRIM(_I32, btRaycastVehicle_getUpAxis, _IDL);

HL_PRIM int HL_NAME(btRaycastVehicle_getForwardAxis)(_ref(btRaycastVehicle)* _this) {
	return _unref(_this)->getForwardAxis();
}
DEFINE_PRIM(_I32, btRaycastVehicle_getForwardAxis, _IDL);

HL_PRIM _ref(btVector3)* HL_NAME(btRaycastVehicle_getForwardVector)(_ref(btRaycastVehicle)* _this) {
	return alloc_ref(new btVector3(_unref(_this)->getForwardVector()),finalize_btVector3);
}
DEFINE_PRIM(_IDL, btRaycastVehicle_getForwardVector, _IDL);

HL_PRIM int HL_NAME(btRaycastVehicle_getUserConstraintType)(_ref(btRaycastVehicle)* _this) {
	return _unref(_this)->getUserConstraintType();
}
DEFINE_PRIM(_I32, btRaycastVehicle_getUserConstraintType, _IDL);

HL_PRIM void HL_NAME(btRaycastVehicle_setUserConstraintType)(_ref(btRaycastVehicle)* _this, int userConstraintType) {
	_unref(_this)->setUserConstraintType(userConstraintType);
}
DEFINE_PRIM(_VOID, btRaycastVehicle_setUserConstraintType, _IDL _I32);

HL_PRIM void HL_NAME(btRaycastVehicle_setUserConstraintId)(_ref(btRaycastVehicle)* _this, int uid) {
	_unref(_this)->setUserConstraintId(uid);
}
DEFINE_PRIM(_VOID, btRaycastVehicle_setUserConstraintId, _IDL _I32);

HL_PRIM int HL_NAME(btRaycastVehicle_getUserConstraintId)(_ref(btRaycastVehicle)* _this) {
	return _unref(_this)->getUserConstraintId();
}
DEFINE_PRIM(_I32, btRaycastVehicle_getUserConstraintId, _IDL);

HL_PRIM _ref(btGhostObject)* HL_NAME(btGhostObject_new0)() {
	return alloc_ref((new btGhostObject()),finalize_btGhostObject);
}
DEFINE_PRIM(_IDL, btGhostObject_new0,);

HL_PRIM int HL_NAME(btGhostObject_getNumOverlappingObjects)(_ref(btGhostObject)* _this) {
	return _unref(_this)->getNumOverlappingObjects();
}
DEFINE_PRIM(_I32, btGhostObject_getNumOverlappingObjects, _IDL);

HL_PRIM _ref(btCollisionObject)* HL_NAME(btGhostObject_getOverlappingObject)(_ref(btGhostObject)* _this, int index) {
	return alloc_ref((_unref(_this)->getOverlappingObject(index)),finalize_btCollisionObject);
}
DEFINE_PRIM(_IDL, btGhostObject_getOverlappingObject, _IDL _I32);

HL_PRIM _ref(btPairCachingGhostObject)* HL_NAME(btPairCachingGhostObject_new0)() {
	return alloc_ref((new btPairCachingGhostObject()),finalize_btPairCachingGhostObject);
}
DEFINE_PRIM(_IDL, btPairCachingGhostObject_new0,);

HL_PRIM _ref(btGhostPairCallback)* HL_NAME(btGhostPairCallback_new0)() {
	return alloc_ref((new btGhostPairCallback()),finalize_btGhostPairCallback);
}
DEFINE_PRIM(_IDL, btGhostPairCallback_new0,);

HL_PRIM _ref(btSoftBodyWorldInfo)* HL_NAME(btSoftBodyWorldInfo_new0)() {
	return alloc_ref((new btSoftBodyWorldInfo()),finalize_btSoftBodyWorldInfo);
}
DEFINE_PRIM(_IDL, btSoftBodyWorldInfo_new0,);

HL_PRIM float HL_NAME(btSoftBodyWorldInfo_get_air_density)( _ref(btSoftBodyWorldInfo)* _this ) {
	return _unref(_this)->air_density;
}
HL_PRIM void HL_NAME(btSoftBodyWorldInfo_set_air_density)( _ref(btSoftBodyWorldInfo)* _this, float value ) {
	_unref(_this)->air_density = (value);
}

HL_PRIM float HL_NAME(btSoftBodyWorldInfo_get_water_density)( _ref(btSoftBodyWorldInfo)* _this ) {
	return _unref(_this)->water_density;
}
HL_PRIM void HL_NAME(btSoftBodyWorldInfo_set_water_density)( _ref(btSoftBodyWorldInfo)* _this, float value ) {
	_unref(_this)->water_density = (value);
}

HL_PRIM float HL_NAME(btSoftBodyWorldInfo_get_water_offset)( _ref(btSoftBodyWorldInfo)* _this ) {
	return _unref(_this)->water_offset;
}
HL_PRIM void HL_NAME(btSoftBodyWorldInfo_set_water_offset)( _ref(btSoftBodyWorldInfo)* _this, float value ) {
	_unref(_this)->water_offset = (value);
}

HL_PRIM float HL_NAME(btSoftBodyWorldInfo_get_m_maxDisplacement)( _ref(btSoftBodyWorldInfo)* _this ) {
	return _unref(_this)->m_maxDisplacement;
}
HL_PRIM void HL_NAME(btSoftBodyWorldInfo_set_m_maxDisplacement)( _ref(btSoftBodyWorldInfo)* _this, float value ) {
	_unref(_this)->m_maxDisplacement = (value);
}

HL_PRIM _ref(btVector3)* HL_NAME(btSoftBodyWorldInfo_get_water_normal)( _ref(btSoftBodyWorldInfo)* _this ) {
	return alloc_ref(new btVector3(_unref(_this)->water_normal),finalize_btVector3);
}
HL_PRIM void HL_NAME(btSoftBodyWorldInfo_set_water_normal)( _ref(btSoftBodyWorldInfo)* _this, _ref(btVector3)* value ) {
	_unref(_this)->water_normal = *_unref(value);
}

HL_PRIM _ref(btBroadphaseInterface)* HL_NAME(btSoftBodyWorldInfo_get_m_broadphase)( _ref(btSoftBodyWorldInfo)* _this ) {
	return alloc_ref(_unref(_this)->m_broadphase,finalize_btBroadphaseInterface);
}
HL_PRIM void HL_NAME(btSoftBodyWorldInfo_set_m_broadphase)( _ref(btSoftBodyWorldInfo)* _this, _ref(btBroadphaseInterface)* value ) {
	_unref(_this)->m_broadphase = _unref(value);
}

HL_PRIM _ref(btDispatcher)* HL_NAME(btSoftBodyWorldInfo_get_m_dispatcher)( _ref(btSoftBodyWorldInfo)* _this ) {
	return alloc_ref(_unref(_this)->m_dispatcher,finalize_btDispatcher);
}
HL_PRIM void HL_NAME(btSoftBodyWorldInfo_set_m_dispatcher)( _ref(btSoftBodyWorldInfo)* _this, _ref(btDispatcher)* value ) {
	_unref(_this)->m_dispatcher = _unref(value);
}

HL_PRIM _ref(btVector3)* HL_NAME(btSoftBodyWorldInfo_get_m_gravity)( _ref(btSoftBodyWorldInfo)* _this ) {
	return alloc_ref(new btVector3(_unref(_this)->m_gravity),finalize_btVector3);
}
HL_PRIM void HL_NAME(btSoftBodyWorldInfo_set_m_gravity)( _ref(btSoftBodyWorldInfo)* _this, _ref(btVector3)* value ) {
	_unref(_this)->m_gravity = *_unref(value);
}

HL_PRIM _ref(btVector3)* HL_NAME(Node_get_m_x)( _ref(btSoftBody::Node)* _this ) {
	return alloc_ref(new btVector3(_unref(_this)->m_x),finalize_btVector3);
}
HL_PRIM void HL_NAME(Node_set_m_x)( _ref(btSoftBody::Node)* _this, _ref(btVector3)* value ) {
	_unref(_this)->m_x = *_unref(value);
}

HL_PRIM _ref(btVector3)* HL_NAME(Node_get_m_n)( _ref(btSoftBody::Node)* _this ) {
	return alloc_ref(new btVector3(_unref(_this)->m_n),finalize_btVector3);
}
HL_PRIM void HL_NAME(Node_set_m_n)( _ref(btSoftBody::Node)* _this, _ref(btVector3)* value ) {
	_unref(_this)->m_n = *_unref(value);
}

HL_PRIM int HL_NAME(tNodeArray_size)(_ref(btSoftBody::tNodeArray)* _this) {
	return _unref(_this)->size();
}
DEFINE_PRIM(_I32, tNodeArray_size, _IDL);

HL_PRIM _ref(btSoftBody::Node)* HL_NAME(tNodeArray_at)(_ref(btSoftBody::tNodeArray)* _this, int n) {
	return alloc_ref(new btSoftBody::Node(_unref(_this)->at(n)),finalize_Node);
}
DEFINE_PRIM(_IDL, tNodeArray_at, _IDL _I32);

HL_PRIM float HL_NAME(Material_get_m_kLST)( _ref(btSoftBody::Material)* _this ) {
	return _unref(_this)->m_kLST;
}
HL_PRIM void HL_NAME(Material_set_m_kLST)( _ref(btSoftBody::Material)* _this, float value ) {
	_unref(_this)->m_kLST = (value);
}

HL_PRIM float HL_NAME(Material_get_m_kAST)( _ref(btSoftBody::Material)* _this ) {
	return _unref(_this)->m_kAST;
}
HL_PRIM void HL_NAME(Material_set_m_kAST)( _ref(btSoftBody::Material)* _this, float value ) {
	_unref(_this)->m_kAST = (value);
}

HL_PRIM float HL_NAME(Material_get_m_kVST)( _ref(btSoftBody::Material)* _this ) {
	return _unref(_this)->m_kVST;
}
HL_PRIM void HL_NAME(Material_set_m_kVST)( _ref(btSoftBody::Material)* _this, float value ) {
	_unref(_this)->m_kVST = (value);
}

HL_PRIM int HL_NAME(Material_get_m_flags)( _ref(btSoftBody::Material)* _this ) {
	return _unref(_this)->m_flags;
}
HL_PRIM void HL_NAME(Material_set_m_flags)( _ref(btSoftBody::Material)* _this, int value ) {
	_unref(_this)->m_flags = (value);
}

HL_PRIM int HL_NAME(tMaterialArray_size)(_ref(btSoftBody::tMaterialArray)* _this) {
	return _unref(_this)->size();
}
DEFINE_PRIM(_I32, tMaterialArray_size, _IDL);

HL_PRIM _ref(btSoftBody::Material)* HL_NAME(tMaterialArray_at)(_ref(btSoftBody::tMaterialArray)* _this, int n) {
	return alloc_ref((_unref(_this)->at(n)),finalize_Material);
}
DEFINE_PRIM(_IDL, tMaterialArray_at, _IDL _I32);

HL_PRIM float HL_NAME(Config_get_kVCF)( _ref(btSoftBody::Config)* _this ) {
	return _unref(_this)->kVCF;
}
HL_PRIM void HL_NAME(Config_set_kVCF)( _ref(btSoftBody::Config)* _this, float value ) {
	_unref(_this)->kVCF = (value);
}

HL_PRIM float HL_NAME(Config_get_kDP)( _ref(btSoftBody::Config)* _this ) {
	return _unref(_this)->kDP;
}
HL_PRIM void HL_NAME(Config_set_kDP)( _ref(btSoftBody::Config)* _this, float value ) {
	_unref(_this)->kDP = (value);
}

HL_PRIM float HL_NAME(Config_get_kDG)( _ref(btSoftBody::Config)* _this ) {
	return _unref(_this)->kDG;
}
HL_PRIM void HL_NAME(Config_set_kDG)( _ref(btSoftBody::Config)* _this, float value ) {
	_unref(_this)->kDG = (value);
}

HL_PRIM float HL_NAME(Config_get_kLF)( _ref(btSoftBody::Config)* _this ) {
	return _unref(_this)->kLF;
}
HL_PRIM void HL_NAME(Config_set_kLF)( _ref(btSoftBody::Config)* _this, float value ) {
	_unref(_this)->kLF = (value);
}

HL_PRIM float HL_NAME(Config_get_kPR)( _ref(btSoftBody::Config)* _this ) {
	return _unref(_this)->kPR;
}
HL_PRIM void HL_NAME(Config_set_kPR)( _ref(btSoftBody::Config)* _this, float value ) {
	_unref(_this)->kPR = (value);
}

HL_PRIM float HL_NAME(Config_get_kVC)( _ref(btSoftBody::Config)* _this ) {
	return _unref(_this)->kVC;
}
HL_PRIM void HL_NAME(Config_set_kVC)( _ref(btSoftBody::Config)* _this, float value ) {
	_unref(_this)->kVC = (value);
}

HL_PRIM float HL_NAME(Config_get_kDF)( _ref(btSoftBody::Config)* _this ) {
	return _unref(_this)->kDF;
}
HL_PRIM void HL_NAME(Config_set_kDF)( _ref(btSoftBody::Config)* _this, float value ) {
	_unref(_this)->kDF = (value);
}

HL_PRIM float HL_NAME(Config_get_kMT)( _ref(btSoftBody::Config)* _this ) {
	return _unref(_this)->kMT;
}
HL_PRIM void HL_NAME(Config_set_kMT)( _ref(btSoftBody::Config)* _this, float value ) {
	_unref(_this)->kMT = (value);
}

HL_PRIM float HL_NAME(Config_get_kCHR)( _ref(btSoftBody::Config)* _this ) {
	return _unref(_this)->kCHR;
}
HL_PRIM void HL_NAME(Config_set_kCHR)( _ref(btSoftBody::Config)* _this, float value ) {
	_unref(_this)->kCHR = (value);
}

HL_PRIM float HL_NAME(Config_get_kKHR)( _ref(btSoftBody::Config)* _this ) {
	return _unref(_this)->kKHR;
}
HL_PRIM void HL_NAME(Config_set_kKHR)( _ref(btSoftBody::Config)* _this, float value ) {
	_unref(_this)->kKHR = (value);
}

HL_PRIM float HL_NAME(Config_get_kSHR)( _ref(btSoftBody::Config)* _this ) {
	return _unref(_this)->kSHR;
}
HL_PRIM void HL_NAME(Config_set_kSHR)( _ref(btSoftBody::Config)* _this, float value ) {
	_unref(_this)->kSHR = (value);
}

HL_PRIM float HL_NAME(Config_get_kAHR)( _ref(btSoftBody::Config)* _this ) {
	return _unref(_this)->kAHR;
}
HL_PRIM void HL_NAME(Config_set_kAHR)( _ref(btSoftBody::Config)* _this, float value ) {
	_unref(_this)->kAHR = (value);
}

HL_PRIM float HL_NAME(Config_get_kSRHR_CL)( _ref(btSoftBody::Config)* _this ) {
	return _unref(_this)->kSRHR_CL;
}
HL_PRIM void HL_NAME(Config_set_kSRHR_CL)( _ref(btSoftBody::Config)* _this, float value ) {
	_unref(_this)->kSRHR_CL = (value);
}

HL_PRIM float HL_NAME(Config_get_kSKHR_CL)( _ref(btSoftBody::Config)* _this ) {
	return _unref(_this)->kSKHR_CL;
}
HL_PRIM void HL_NAME(Config_set_kSKHR_CL)( _ref(btSoftBody::Config)* _this, float value ) {
	_unref(_this)->kSKHR_CL = (value);
}

HL_PRIM float HL_NAME(Config_get_kSSHR_CL)( _ref(btSoftBody::Config)* _this ) {
	return _unref(_this)->kSSHR_CL;
}
HL_PRIM void HL_NAME(Config_set_kSSHR_CL)( _ref(btSoftBody::Config)* _this, float value ) {
	_unref(_this)->kSSHR_CL = (value);
}

HL_PRIM float HL_NAME(Config_get_kSR_SPLT_CL)( _ref(btSoftBody::Config)* _this ) {
	return _unref(_this)->kSR_SPLT_CL;
}
HL_PRIM void HL_NAME(Config_set_kSR_SPLT_CL)( _ref(btSoftBody::Config)* _this, float value ) {
	_unref(_this)->kSR_SPLT_CL = (value);
}

HL_PRIM float HL_NAME(Config_get_kSK_SPLT_CL)( _ref(btSoftBody::Config)* _this ) {
	return _unref(_this)->kSK_SPLT_CL;
}
HL_PRIM void HL_NAME(Config_set_kSK_SPLT_CL)( _ref(btSoftBody::Config)* _this, float value ) {
	_unref(_this)->kSK_SPLT_CL = (value);
}

HL_PRIM float HL_NAME(Config_get_kSS_SPLT_CL)( _ref(btSoftBody::Config)* _this ) {
	return _unref(_this)->kSS_SPLT_CL;
}
HL_PRIM void HL_NAME(Config_set_kSS_SPLT_CL)( _ref(btSoftBody::Config)* _this, float value ) {
	_unref(_this)->kSS_SPLT_CL = (value);
}

HL_PRIM float HL_NAME(Config_get_maxvolume)( _ref(btSoftBody::Config)* _this ) {
	return _unref(_this)->maxvolume;
}
HL_PRIM void HL_NAME(Config_set_maxvolume)( _ref(btSoftBody::Config)* _this, float value ) {
	_unref(_this)->maxvolume = (value);
}

HL_PRIM float HL_NAME(Config_get_timescale)( _ref(btSoftBody::Config)* _this ) {
	return _unref(_this)->timescale;
}
HL_PRIM void HL_NAME(Config_set_timescale)( _ref(btSoftBody::Config)* _this, float value ) {
	_unref(_this)->timescale = (value);
}

HL_PRIM int HL_NAME(Config_get_viterations)( _ref(btSoftBody::Config)* _this ) {
	return _unref(_this)->viterations;
}
HL_PRIM void HL_NAME(Config_set_viterations)( _ref(btSoftBody::Config)* _this, int value ) {
	_unref(_this)->viterations = (value);
}

HL_PRIM int HL_NAME(Config_get_piterations)( _ref(btSoftBody::Config)* _this ) {
	return _unref(_this)->piterations;
}
HL_PRIM void HL_NAME(Config_set_piterations)( _ref(btSoftBody::Config)* _this, int value ) {
	_unref(_this)->piterations = (value);
}

HL_PRIM int HL_NAME(Config_get_diterations)( _ref(btSoftBody::Config)* _this ) {
	return _unref(_this)->diterations;
}
HL_PRIM void HL_NAME(Config_set_diterations)( _ref(btSoftBody::Config)* _this, int value ) {
	_unref(_this)->diterations = (value);
}

HL_PRIM int HL_NAME(Config_get_citerations)( _ref(btSoftBody::Config)* _this ) {
	return _unref(_this)->citerations;
}
HL_PRIM void HL_NAME(Config_set_citerations)( _ref(btSoftBody::Config)* _this, int value ) {
	_unref(_this)->citerations = (value);
}

HL_PRIM int HL_NAME(Config_get_collisions)( _ref(btSoftBody::Config)* _this ) {
	return _unref(_this)->collisions;
}
HL_PRIM void HL_NAME(Config_set_collisions)( _ref(btSoftBody::Config)* _this, int value ) {
	_unref(_this)->collisions = (value);
}

HL_PRIM _ref(btSoftBody)* HL_NAME(btSoftBody_new4)(_ref(btSoftBodyWorldInfo)* worldInfo, int node_count, _ref(btVector3)* x, float* m) {
	return alloc_ref((new btSoftBody(_unref(worldInfo), node_count, _unref(x), m)),finalize_btSoftBody);
}
DEFINE_PRIM(_IDL, btSoftBody_new4, _IDL _I32 _IDL _BYTES);

HL_PRIM _ref(btSoftBody::Config)* HL_NAME(btSoftBody_get_m_cfg)( _ref(btSoftBody)* _this ) {
	return alloc_ref(new btSoftBody::Config(_unref(_this)->m_cfg),finalize_Config);
}
HL_PRIM void HL_NAME(btSoftBody_set_m_cfg)( _ref(btSoftBody)* _this, _ref(btSoftBody::Config)* value ) {
	_unref(_this)->m_cfg = *_unref(value);
}

HL_PRIM _ref(btSoftBody::tNodeArray)* HL_NAME(btSoftBody_get_m_nodes)( _ref(btSoftBody)* _this ) {
	return alloc_ref(new btSoftBody::tNodeArray(_unref(_this)->m_nodes),finalize_tNodeArray);
}
HL_PRIM void HL_NAME(btSoftBody_set_m_nodes)( _ref(btSoftBody)* _this, _ref(btSoftBody::tNodeArray)* value ) {
	_unref(_this)->m_nodes = *_unref(value);
}

HL_PRIM _ref(btSoftBody::tMaterialArray)* HL_NAME(btSoftBody_get_m_materials)( _ref(btSoftBody)* _this ) {
	return alloc_ref(new btSoftBody::tMaterialArray(_unref(_this)->m_materials),finalize_tMaterialArray);
}
HL_PRIM void HL_NAME(btSoftBody_set_m_materials)( _ref(btSoftBody)* _this, _ref(btSoftBody::tMaterialArray)* value ) {
	_unref(_this)->m_materials = *_unref(value);
}

HL_PRIM bool HL_NAME(btSoftBody_checkLink)(_ref(btSoftBody)* _this, int node0, int node1) {
	return _unref(_this)->checkLink(node0, node1);
}
DEFINE_PRIM(_BOOL, btSoftBody_checkLink, _IDL _I32 _I32);

HL_PRIM bool HL_NAME(btSoftBody_checkFace)(_ref(btSoftBody)* _this, int node0, int node1, int node2) {
	return _unref(_this)->checkFace(node0, node1, node2);
}
DEFINE_PRIM(_BOOL, btSoftBody_checkFace, _IDL _I32 _I32 _I32);

HL_PRIM _ref(btSoftBody::Material)* HL_NAME(btSoftBody_appendMaterial)(_ref(btSoftBody)* _this) {
	return alloc_ref((_unref(_this)->appendMaterial()),finalize_Material);
}
DEFINE_PRIM(_IDL, btSoftBody_appendMaterial, _IDL);

HL_PRIM void HL_NAME(btSoftBody_appendNode)(_ref(btSoftBody)* _this, _ref(btVector3)* x, float m) {
	_unref(_this)->appendNode(*_unref(x), m);
}
DEFINE_PRIM(_VOID, btSoftBody_appendNode, _IDL _IDL _F32);

HL_PRIM void HL_NAME(btSoftBody_appendLink)(_ref(btSoftBody)* _this, int node0, int node1, _ref(btSoftBody::Material)* mat, bool bcheckexist) {
	_unref(_this)->appendLink(node0, node1, _unref(mat), bcheckexist);
}
DEFINE_PRIM(_VOID, btSoftBody_appendLink, _IDL _I32 _I32 _IDL _BOOL);

HL_PRIM void HL_NAME(btSoftBody_appendFace)(_ref(btSoftBody)* _this, int node0, int node1, int node2, _ref(btSoftBody::Material)* mat) {
	_unref(_this)->appendFace(node0, node1, node2, _unref(mat));
}
DEFINE_PRIM(_VOID, btSoftBody_appendFace, _IDL _I32 _I32 _I32 _IDL);

HL_PRIM void HL_NAME(btSoftBody_appendTetra)(_ref(btSoftBody)* _this, int node0, int node1, int node2, int node3, _ref(btSoftBody::Material)* mat) {
	_unref(_this)->appendTetra(node0, node1, node2, node3, _unref(mat));
}
DEFINE_PRIM(_VOID, btSoftBody_appendTetra, _IDL _I32 _I32 _I32 _I32 _IDL);

HL_PRIM void HL_NAME(btSoftBody_appendAnchor)(_ref(btSoftBody)* _this, int node, _ref(btRigidBody)* body, bool disableCollisionBetweenLinkedBodies, float influence) {
	_unref(_this)->appendAnchor(node, _unref(body), disableCollisionBetweenLinkedBodies, influence);
}
DEFINE_PRIM(_VOID, btSoftBody_appendAnchor, _IDL _I32 _IDL _BOOL _F32);

HL_PRIM float HL_NAME(btSoftBody_getTotalMass)(_ref(btSoftBody)* _this) {
	return _unref(_this)->getTotalMass();
}
DEFINE_PRIM(_F32, btSoftBody_getTotalMass, _IDL);

HL_PRIM void HL_NAME(btSoftBody_setTotalMass)(_ref(btSoftBody)* _this, float mass, bool fromfaces) {
	_unref(_this)->setTotalMass(mass, fromfaces);
}
DEFINE_PRIM(_VOID, btSoftBody_setTotalMass, _IDL _F32 _BOOL);

HL_PRIM void HL_NAME(btSoftBody_setMass)(_ref(btSoftBody)* _this, int node, float mass) {
	_unref(_this)->setMass(node, mass);
}
DEFINE_PRIM(_VOID, btSoftBody_setMass, _IDL _I32 _F32);

HL_PRIM void HL_NAME(btSoftBody_transform)(_ref(btSoftBody)* _this, _ref(btTransform)* trs) {
	_unref(_this)->transform(*_unref(trs));
}
DEFINE_PRIM(_VOID, btSoftBody_transform, _IDL _IDL);

HL_PRIM void HL_NAME(btSoftBody_translate)(_ref(btSoftBody)* _this, _ref(btVector3)* trs) {
	_unref(_this)->translate(*_unref(trs));
}
DEFINE_PRIM(_VOID, btSoftBody_translate, _IDL _IDL);

HL_PRIM void HL_NAME(btSoftBody_rotate)(_ref(btSoftBody)* _this, _ref(btQuaternion)* rot) {
	_unref(_this)->rotate(*_unref(rot));
}
DEFINE_PRIM(_VOID, btSoftBody_rotate, _IDL _IDL);

HL_PRIM void HL_NAME(btSoftBody_scale)(_ref(btSoftBody)* _this, _ref(btVector3)* scl) {
	_unref(_this)->scale(*_unref(scl));
}
DEFINE_PRIM(_VOID, btSoftBody_scale, _IDL _IDL);

HL_PRIM int HL_NAME(btSoftBody_generateClusters)(_ref(btSoftBody)* _this, int k, vdynamic* maxiterations) {
	if( !maxiterations )
		return _unref(_this)->generateClusters(k);
	else
		return _unref(_this)->generateClusters(k, maxiterations->v.i);
}
DEFINE_PRIM(_I32, btSoftBody_generateClusters, _IDL _I32 _NULL(_I32));

HL_PRIM _ref(btSoftBody)* HL_NAME(btSoftBody_upcast)(_ref(btSoftBody)* _this, _ref(btCollisionObject)* colObj) {
	return alloc_ref((_unref(_this)->upcast(_unref(colObj))),finalize_btSoftBody);
}
DEFINE_PRIM(_IDL, btSoftBody_upcast, _IDL _IDL);

HL_PRIM _ref(btSoftBodyRigidBodyCollisionConfiguration)* HL_NAME(btSoftBodyRigidBodyCollisionConfiguration_new1)(_ref(btDefaultCollisionConstructionInfo)* info) {
	if( !info )
		return alloc_ref((new btSoftBodyRigidBodyCollisionConfiguration()),finalize_btSoftBodyRigidBodyCollisionConfiguration);
	else
		return alloc_ref((new btSoftBodyRigidBodyCollisionConfiguration(*_unref(info))),finalize_btSoftBodyRigidBodyCollisionConfiguration);
}
DEFINE_PRIM(_IDL, btSoftBodyRigidBodyCollisionConfiguration_new1, _IDL);

HL_PRIM _ref(btDefaultSoftBodySolver)* HL_NAME(btDefaultSoftBodySolver_new0)() {
	return alloc_ref((new btDefaultSoftBodySolver()),finalize_btDefaultSoftBodySolver);
}
DEFINE_PRIM(_IDL, btDefaultSoftBodySolver_new0,);

HL_PRIM int HL_NAME(btSoftBodyArray_size)(_ref(btSoftBodyArray)* _this) {
	return _unref(_this)->size();
}
DEFINE_PRIM(_I32, btSoftBodyArray_size, _IDL);

HL_PRIM _ref(btSoftBody)* HL_NAME(btSoftBodyArray_at)(_ref(btSoftBodyArray)* _this, int n) {
	return alloc_ref_const((_unref(_this)->at(n)),finalize_btSoftBody);
}
DEFINE_PRIM(_IDL, btSoftBodyArray_at, _IDL _I32);

HL_PRIM _ref(btSoftRigidDynamicsWorld)* HL_NAME(btSoftRigidDynamicsWorld_new5)(_ref(btDispatcher)* dispatcher, _ref(btBroadphaseInterface)* pairCache, _ref(btConstraintSolver)* constraintSolver, _ref(btCollisionConfiguration)* collisionConfiguration, _ref(btSoftBodySolver)* softBodySolver) {
	return alloc_ref((new btSoftRigidDynamicsWorld(_unref(dispatcher), _unref(pairCache), _unref(constraintSolver), _unref(collisionConfiguration), _unref(softBodySolver))),finalize_btSoftRigidDynamicsWorld);
}
DEFINE_PRIM(_IDL, btSoftRigidDynamicsWorld_new5, _IDL _IDL _IDL _IDL _IDL);

HL_PRIM void HL_NAME(btSoftRigidDynamicsWorld_addSoftBody)(_ref(btSoftRigidDynamicsWorld)* _this, _ref(btSoftBody)* body, short collisionFilterGroup, short collisionFilterMask) {
	_unref(_this)->addSoftBody(_unref(body), collisionFilterGroup, collisionFilterMask);
}
DEFINE_PRIM(_VOID, btSoftRigidDynamicsWorld_addSoftBody, _IDL _IDL _I16 _I16);

HL_PRIM void HL_NAME(btSoftRigidDynamicsWorld_removeSoftBody)(_ref(btSoftRigidDynamicsWorld)* _this, _ref(btSoftBody)* body) {
	_unref(_this)->removeSoftBody(_unref(body));
}
DEFINE_PRIM(_VOID, btSoftRigidDynamicsWorld_removeSoftBody, _IDL _IDL);

HL_PRIM void HL_NAME(btSoftRigidDynamicsWorld_removeCollisionObject)(_ref(btSoftRigidDynamicsWorld)* _this, _ref(btCollisionObject)* collisionObject) {
	_unref(_this)->removeCollisionObject(_unref(collisionObject));
}
DEFINE_PRIM(_VOID, btSoftRigidDynamicsWorld_removeCollisionObject, _IDL _IDL);

HL_PRIM _ref(btSoftBodyWorldInfo)* HL_NAME(btSoftRigidDynamicsWorld_getWorldInfo)(_ref(btSoftRigidDynamicsWorld)* _this) {
	return alloc_ref(new btSoftBodyWorldInfo(_unref(_this)->getWorldInfo()),finalize_btSoftBodyWorldInfo);
}
DEFINE_PRIM(_IDL, btSoftRigidDynamicsWorld_getWorldInfo, _IDL);

HL_PRIM _ref(btSoftBodyArray)* HL_NAME(btSoftRigidDynamicsWorld_getSoftBodyArray)(_ref(btSoftRigidDynamicsWorld)* _this) {
	return alloc_ref(new btSoftBodyArray(_unref(_this)->getSoftBodyArray()),finalize_btSoftBodyArray);
}
DEFINE_PRIM(_IDL, btSoftRigidDynamicsWorld_getSoftBodyArray, _IDL);

HL_PRIM _ref(btSoftBodyHelpers)* HL_NAME(btSoftBodyHelpers_new0)() {
	return alloc_ref((new btSoftBodyHelpers()),finalize_btSoftBodyHelpers);
}
DEFINE_PRIM(_IDL, btSoftBodyHelpers_new0,);

HL_PRIM _ref(btSoftBody)* HL_NAME(btSoftBodyHelpers_CreateRope)(_ref(btSoftBodyHelpers)* _this, _ref(btSoftBodyWorldInfo)* worldInfo, _ref(btVector3)* from, _ref(btVector3)* to, int res, int fixeds) {
	return alloc_ref((_unref(_this)->CreateRope(*_unref(worldInfo), *_unref(from), *_unref(to), res, fixeds)),finalize_btSoftBody);
}
DEFINE_PRIM(_IDL, btSoftBodyHelpers_CreateRope, _IDL _IDL _IDL _IDL _I32 _I32);

HL_PRIM _ref(btSoftBody)* HL_NAME(btSoftBodyHelpers_CreatePatch)(_ref(btSoftBodyHelpers)* _this, _ref(btSoftBodyWorldInfo)* worldInfo, _ref(btVector3)* corner00, _ref(btVector3)* corner10, _ref(btVector3)* corner01, _ref(btVector3)* corner11, int resx, int resy, int fixeds, bool gendiags) {
	return alloc_ref((_unref(_this)->CreatePatch(*_unref(worldInfo), *_unref(corner00), *_unref(corner10), *_unref(corner01), *_unref(corner11), resx, resy, fixeds, gendiags)),finalize_btSoftBody);
}
DEFINE_PRIM(_IDL, btSoftBodyHelpers_CreatePatch, _IDL _IDL _IDL _IDL _IDL _IDL _I32 _I32 _I32 _BOOL);

HL_PRIM _ref(btSoftBody)* HL_NAME(btSoftBodyHelpers_CreatePatchUV)(_ref(btSoftBodyHelpers)* _this, _ref(btSoftBodyWorldInfo)* worldInfo, _ref(btVector3)* corner00, _ref(btVector3)* corner10, _ref(btVector3)* corner01, _ref(btVector3)* corner11, int resx, int resy, int fixeds, bool gendiags, float* tex_coords) {
	return alloc_ref((_unref(_this)->CreatePatchUV(*_unref(worldInfo), *_unref(corner00), *_unref(corner10), *_unref(corner01), *_unref(corner11), resx, resy, fixeds, gendiags, tex_coords)),finalize_btSoftBody);
}
DEFINE_PRIM(_IDL, btSoftBodyHelpers_CreatePatchUV, _IDL _IDL _IDL _IDL _IDL _IDL _I32 _I32 _I32 _BOOL _BYTES);

HL_PRIM _ref(btSoftBody)* HL_NAME(btSoftBodyHelpers_CreateEllipsoid)(_ref(btSoftBodyHelpers)* _this, _ref(btSoftBodyWorldInfo)* worldInfo, _ref(btVector3)* center, _ref(btVector3)* radius, int res) {
	return alloc_ref((_unref(_this)->CreateEllipsoid(*_unref(worldInfo), *_unref(center), *_unref(radius), res)),finalize_btSoftBody);
}
DEFINE_PRIM(_IDL, btSoftBodyHelpers_CreateEllipsoid, _IDL _IDL _IDL _IDL _I32);

HL_PRIM _ref(btSoftBody)* HL_NAME(btSoftBodyHelpers_CreateFromTriMesh)(_ref(btSoftBodyHelpers)* _this, _ref(btSoftBodyWorldInfo)* worldInfo, float* vertices, int* triangles, int ntriangles, bool randomizeConstraints) {
	return alloc_ref((_unref(_this)->CreateFromTriMesh(*_unref(worldInfo), vertices, triangles, ntriangles, randomizeConstraints)),finalize_btSoftBody);
}
DEFINE_PRIM(_IDL, btSoftBodyHelpers_CreateFromTriMesh, _IDL _IDL _BYTES _BYTES _I32 _BOOL);

HL_PRIM _ref(btSoftBody)* HL_NAME(btSoftBodyHelpers_CreateFromConvexHull)(_ref(btSoftBodyHelpers)* _this, _ref(btSoftBodyWorldInfo)* worldInfo, _ref(btVector3)* vertices, int nvertices, bool randomizeConstraints) {
	return alloc_ref((_unref(_this)->CreateFromConvexHull(*_unref(worldInfo), _unref(vertices), nvertices, randomizeConstraints)),finalize_btSoftBody);
}
DEFINE_PRIM(_IDL, btSoftBodyHelpers_CreateFromConvexHull, _IDL _IDL _IDL _I32 _BOOL);

