
class Main extends hxd.App {

	var world : bullet.World;
	var bodies : Array<{ b : bullet.Body, m : h3d.scene.Mesh }> = [];

	override function init() {
		world = new bullet.World();
		world.setGravity(0,0,-9.81);

		var floor = new bullet.Body(bullet.Shape.createBox(100,100,1),0);
		world.addRigidBody(floor);

		var floorGfx = new h3d.prim.Cube(100, 100, 1, true);
		var floorMesh = new h3d.scene.Mesh(floorGfx, s3d);
		floorMesh.material.color.setColor(0x800000);
		bodies.push({ b : floor, m : floorMesh });

		new h3d.scene.DirLight(new h3d.Vector(1, 2, -4), s3d);

		var shapes = [bullet.Shape.createSphere(0.5), bullet.Shape.createBox(1,1,1)];
		var prims = [new h3d.prim.Sphere(0.5), new h3d.prim.Cube(1, 1, 1, true)];
		prims[1].unindex();

		var comp = bullet.Shape.createCompound([
			{ shape : shapes[0], mass : 1, position : new h3d.col.Point(0, 0, 0), rotation : new h3d.Quat() },
			{ shape : shapes[1], mass : 1, position : new h3d.col.Point(0, 0, 1), rotation : new h3d.Quat() }
		]);
		shapes.push(comp.shape);
		var c = new h3d.prim.Cube(1, 1, 2, true);
		c.unindex();
		prims.push(c);

		for( p in prims )
			p.addNormals();
		for( i in 0...100 ) {
			var id = Std.random(shapes.length);
			var m = new h3d.scene.Mesh(prims[id], s3d);
			m.x = Math.random() * 10;
			m.y = Math.random() * 10;
			m.z = 2 + Math.random() * 10;

			var mt = new h3d.Matrix();
			mt.identity();
			mt.colorHue(Math.random() * Math.PI * 2);
			m.material.color.set(0.5, 0.3, 0);
			m.material.color.transform(mt);

			var b = new bullet.Body(shapes[id], 0.5);
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