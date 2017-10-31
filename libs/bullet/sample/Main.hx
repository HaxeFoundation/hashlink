
class Main extends hxd.App {

	var world : bullet.World;
	var bodies : Array<{ b : bullet.Body, m : h3d.scene.Mesh }> = [];

	override function init() {
		world = new bullet.World();
		world.setGravity(0,0,-9.81);

		var floor = new bullet.Body(bullet.Shape.createBox(100,100,1),0);
		world.addRigidBody(floor);

		var floorGfx = new h3d.prim.Cube(100, 100, 1);
		floorGfx.translate( -50, -50, -0.5);
		var floorMesh = new h3d.scene.Mesh(floorGfx, s3d);
		floorMesh.material.color.setColor(0x800000);
		bodies.push({ b : floor, m : floorMesh });

		new h3d.scene.DirLight(new h3d.Vector(1, 2, -4));

		var sp = bullet.Shape.createSphere(1);
		var prim = new h3d.prim.Sphere(1,10,10);
		prim.addNormals();
		for( i in 0...20 ) {
			var m = new h3d.scene.Mesh(prim, s3d);
			m.x = Math.random() * 10;
			m.y = Math.random() * 10;
			m.z = 2 + Math.random() * 10;
			var b = new bullet.Body(sp, 0.5);
			b.setTransform(new h3d.col.Point(m.x, m.y, m.z));
			world.addRigidBody(b);
			bodies.push({ b : b, m : m });
		}

		for( b in bodies ) {
			b.m.material.mainPass.enableLights = true;
			b.m.material.shadows = true;
		}

		new h3d.scene.CameraController(80, s3d);
	}

	override function update(dt:Float) {
		world.stepSimulation(dt / 60, 10);
		for( b in bodies ) {
			var pos = b.b.position;
			var q = b.b.rotation;
			b.m.x = pos.x;
			b.m.y = pos.y;
			b.m.z = pos.z;
			b.m.setRotationQuat(q);
		}
	}

	static function main() {
		new Main();
	}

}