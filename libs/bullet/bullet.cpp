#define HL_NAME(x) bullet_##x
#include <hl.h>
#ifdef _WIN32
#pragma warning(disable:4305)
#pragma warning(disable:4244)
#endif
#include <btBulletDynamicsCommon.h>

template <typename T> struct pref {
	void *finalize;
	T *value;
};

typedef pref<btDiscreteDynamicsWorld> World;
typedef pref<btCollisionShape> Shape;
typedef pref<btRigidBody> Body;

template<typename T> pref<T> *alloc_ref( T *value, void (*finalize)( pref<T> * ) ) {
	pref<T> *r = (pref<T>*)hl_gc_alloc_finalizer(sizeof(r));
	r->finalize = finalize;
	r->value = value;
	return r;
}

// --- WORLD ----------------------------------------------------------------

static void finalize_world( World *p ) {
	btCollisionDispatcher *dispatch = (btCollisionDispatcher*)p->value->getDispatcher();	
	delete dispatch->getCollisionConfiguration();
	delete dispatch;
	delete p->value->getBroadphase();
	delete p->value->getConstraintSolver();
	delete p->value;
}

HL_PRIM World *HL_NAME(create_world)() {
	btDefaultCollisionConfiguration *config = new btDefaultCollisionConfiguration();
	btCollisionDispatcher *dispatch = new btCollisionDispatcher(config);
	btDbvtBroadphase *broad = new btDbvtBroadphase();
	btSequentialImpulseConstraintSolver *solver = new btSequentialImpulseConstraintSolver();
	btDiscreteDynamicsWorld *world = new btDiscreteDynamicsWorld(dispatch, broad, solver, config);
	return alloc_ref(world, finalize_world);
}

HL_PRIM void HL_NAME(set_gravity)( World *w, double x, double y, double z ) {
	w->value->setGravity(btVector3(x,y,z));
}

HL_PRIM void HL_NAME(step_simulation)( World *w, double time, int iterations ) {
	w->value->stepSimulation(time,iterations);
}

HL_PRIM void HL_NAME(add_rigid_body)( World *w, Body *b ) {
	w->value->addRigidBody(b->value);
}

HL_PRIM void HL_NAME(remove_rigid_body)( World *w, Body *b ) {
	w->value->removeRigidBody(b->value);
}

#define _WORLD _ABSTRACT(bworld)
#define _SHAPE _ABSTRACT(bshape)
#define _BODY _ABSTRACT(bbody)

DEFINE_PRIM(_WORLD, create_world, _NO_ARG);
DEFINE_PRIM(_VOID, set_gravity, _WORLD _F64 _F64 _F64);
DEFINE_PRIM(_VOID, step_simulation, _WORLD _F64 _I32);

DEFINE_PRIM(_VOID, add_rigid_body, _WORLD _BODY);
DEFINE_PRIM(_VOID, remove_rigid_body, _WORLD _BODY);

// --- SHAPES ----------------------------------------------------------------

static void finalize_shape( Shape *s ) {
	delete s->value;
}

HL_PRIM Shape *HL_NAME(create_box_shape)( double sizeX, double sizeY, double sizeZ ) {
	btCollisionShape *s = new btBoxShape(btVector3(sizeX*0.5,sizeY*0.5,sizeZ*0.5));
	return alloc_ref(s, finalize_shape);
}

HL_PRIM Shape *HL_NAME(create_sphere_shape)( double radius ) {
	btCollisionShape *s = new btSphereShape(radius);
	return alloc_ref(s, finalize_shape);
}

DEFINE_PRIM(_SHAPE, create_box_shape, _F64 _F64 _F64);
DEFINE_PRIM(_SHAPE, create_sphere_shape, _F64);

// --- RIGID BODY ----------------------------------------------------------------

static void finalize_body( Body *b ) {
	delete b->value->getMotionState();
	delete b->value;
}

HL_PRIM Body *HL_NAME(create_body)( Shape *shape, double mass ) {
	btVector3 inertia(0, 0, 0);
	btDefaultMotionState* state = new btDefaultMotionState();
	if( mass != 0 ) shape->value->calculateLocalInertia(mass,inertia);
	btRigidBody::btRigidBodyConstructionInfo info(mass, state, shape->value, inertia);
	return alloc_ref(new btRigidBody(info), finalize_body);
}

HL_PRIM void HL_NAME(set_body_position)( Body *b, double x, double y, double z ) {
	btTransform t = b->value->getCenterOfMassTransform();
	t.setOrigin(btVector3(x,y,z));
	b->value->setCenterOfMassTransform(t);
}

HL_PRIM void HL_NAME(set_body_rotation)( Body *b, double x, double y, double z, double w ) {
	btTransform t = b->value->getCenterOfMassTransform();
	t.setRotation(btQuaternion(x,y,z,w));
	b->value->setCenterOfMassTransform(t);
}

HL_PRIM void HL_NAME(get_body_transform)( Body *b, double *data ) {
	btTransform t = b->value->getCenterOfMassTransform();
	btVector3 p = t.getOrigin();
	btQuaternion q = t.getRotation();
	*data++ = p.x();
	*data++ = p.y();
	*data++ = p.z();
	*data++ = q.x();
	*data++ = q.y();
	*data++ = q.z();
	*data++ = q.w();
}

DEFINE_PRIM(_BODY, create_body, _SHAPE _F64);
DEFINE_PRIM(_VOID, set_body_position, _BODY _F64 _F64 _F64);
DEFINE_PRIM(_VOID, set_body_rotation, _BODY _F64 _F64 _F64 _F64);
DEFINE_PRIM(_VOID, get_body_transform, _BODY _BYTES);
