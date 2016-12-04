@:enum abstract Command(Int) {
	public var Run = 0;
	public var Pause = 1;
	public var Resume = 2;
	public var Stop = 3;
	public var Stack = 4;
	public inline function new(v:Int) {
		this = v;
	}
	public inline function toInt() : Int {
		return this;
	}
}

class Debugger {

	static function error( msg ) {
		Sys.stderr().writeString(msg + "\n");
		Sys.exit(1);
	}

	static function main() {
		var args = Sys.args();
		var defaulPort = 5001;
		var file = null;
		while( args.length > 0 && args[0].charCodeAt(0) == '-'.code ) {
			var param = args.shift();
			switch( param ) {
			case "-port":
				param = args.shift();
				if( param == null || (defaulPort = Std.parseInt(param)) == 0 )
					error("Require port int value");
			default:
				error("Unsupported parameter " + param);
			}
		}
		file = args.shift();
		if( file == null ) {
			Sys.println("hldebug [-port <port>] [-path <path>] <file.hl> <args>");
			Sys.exit(1);
		}
		if( !sys.FileSystem.exists(file) )
			error(file+" not found");
		var content = sys.io.File.getBytes(file);
		var code = new HLReader(false).read(new haxe.io.BytesInput(content));
		var p = new sys.io.Process("hl", ["--debug", "" + defaulPort, "--debug-wait", file]);

		function dumpProcessOut() {
			p.kill();
			Sys.stderr().writeString(p.stderr.readAll().toString());
			Sys.print(p.stdout.readAll().toString());
		}

		var sock = new sys.net.Socket();
		try {
			sock.connect(new sys.net.Host("127.0.0.1"), defaulPort);
		} catch( e : Dynamic ) {
			dumpProcessOut();
			error("Could not connected to debugger");
		}

		sock.setFastSend(true);
		function send( v : Command ) {
			sock.output.writeByte(v.toInt());
		}

		var stdin = Sys.stdin();
		while( true ) {

			var ecode = p.exitCode(false);
			if( ecode != null ) {
				dumpProcessOut();
				error("Process exit with code " + ecode);
			}

			var r = stdin.readLine();
			var args = r.split(" ");
			switch( args.shift() ) {
			case "exit":
				send(Stop);
				Sys.sleep(0.1);
				dumpProcessOut();
				break;
			case "run":
				send(Run);
			case "pause":
				send(Pause);
			case "resume":
				send(Resume);
			case "bt":
				send(Stack);
				var size = sock.input.readInt32();
				Sys.println('[$size]');
				for( i in 0...size ) {
					var fidx = sock.input.readInt32();
					var fpos = sock.input.readInt32();
					if( fidx < 0 )
						Sys.println("????");
					else {
						var f = code.functions[fidx];
						var file = code.debugFiles[f.debug[fpos * 2]];
						var line = f.debug[fpos * 2 + 1];
						Sys.println(file+":" + line);
					}
				}
			default:
				Sys.println("> Unknown command " + r);
			}
		}
	}

}