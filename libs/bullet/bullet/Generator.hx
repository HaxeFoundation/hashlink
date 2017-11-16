package bullet;

#if eval
class Generator {

	static var INCLUDE = "
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

";

	static var options = { idl : "bullet.idl", libName : "bullet", includeCode : INCLUDE, autoGC : true };

	public static function buildHL() {
		webidl.HLGen.generate(options);
	}

	public static function getFiles() {
		var prj = new haxe.xml.Fast(Xml.parse(sys.io.File.getContent("bullet.vcxproj.filters")).firstElement());
		var sources = [];
		for( i in prj.elements )
			if( i.name == "ItemGroup" )
				for( f in i.elements ) {
					if( f.name != "ClCompile" ) continue;
					var fname = f.att.Include.split("\\").join("/");
					sources.push(fname);
				}
		sources.remove("bullet.cpp");
		return sources;
	}

	public static function buildJS() {
		// ammo.js params
		var defines = ["NO_EXIT_RUNTIME=1", "NO_FILESYSTEM=1", "AGGRESSIVE_VARIABLE_ELIMINATION=1", "ELIMINATE_DUPLICATE_FUNCTIONS=1", "NO_DYNAMIC_EXECUTION=1"];
		var params = ["-O3", "--llvm-lto", "1", "-I", "../../include/bullet/src"];
		for( d in defines ) {
			params.push("-s");
			params.push(d);
		}
		webidl.JSGen.compile(options, getFiles(), params);
	}

}
#end