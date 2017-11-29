class Test extends hxd.App {

	override function render(e) {
		var tex = new h3d.mat.Texture(800,600);
		tex.clear(0xFF202020);
		engine.pushTarget(tex);
		super.render(e);
		engine.popTarget();
		
		hxd.File.saveBytes("output.png",tex.capturePixels().toPNG());
		Sys.exit(0);	
	}

	override function init() {
		var c = new h3d.prim.Cube();
		c.unindex();
		c.addNormals();
		new h3d.scene.DirLight(new h3d.Vector(-1,-2,-6),s3d);
		var m = new h3d.scene.Mesh(c, s3d);
		m.material.mainPass.enableLights = true;
		m.material.color.setColor(0xFF704020);		
	}

	static function main() {
		new Test();
	}

}