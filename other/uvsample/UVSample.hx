import hl.uv.*;

class UVSample {

	static var T0 = haxe.Timer.stamp();

	static function log( msg : String ) {
		Sys.println("["+Std.int((haxe.Timer.stamp() - T0) * 100)+"] "+msg);
	}

	static function main() {
		var loop = Loop.getDefault();
		var tcp = new Tcp(loop);

		/*
		tcp.connect(new sys.net.Host("google.com"), 80, function(b) {

			log("Connected=" + b);
			var bytes = haxe.io.Bytes.ofString("GET / HTTP/1.0\r\nHost: www.google.com\r\n\r\n");
			tcp.write(bytes, function(b) trace("Sent=" + b));
			var buf = new haxe.io.BytesBuffer();
			tcp.readStart(function(bytes:hl.Bytes, len) {
				if( len < 0 ) {
					var str = buf.getBytes().toString();
					if( str.length > 1000 )
						str = str.substr(0, 500) + "\n...\n" + str.substr(str.length - 500);
					log("#" + str + "#");
					tcp.readStop();
					return;
				}
				log("Read="+len);
				var bytes = bytes.toBytes(len);
				buf.addBytes(bytes, 0, len);
			});

		});
		*/

		var host = new sys.net.Host("localhost");
		var port = 6001;

		var totR = 0, totW = 0, totRB = 0;

		log("Starting server");
		tcp.bind(host, port);
		tcp.listen(5, function() {

			log("Client connected");
			var s = tcp.accept();
			s.readStart(function(bytes) {
				if( bytes == null ) {
					s.close();
					return;
				}
				totR += bytes.length;
				// write back
				s.write(bytes, function(b) if( !b ) throw "Write failure");
			});

		});


		function startClient() {

			var numbers = [];
			var client = new Tcp(loop);
			//log("Connecting...");
			client.connect(host, port, function(b) {
				//log("Connected to server");


				function send() {
					var b = haxe.io.Bytes.alloc(1);
					var k = Std.random(255);
					numbers.push(k);
					totW++;
					b.set(0, k);
					client.write(b, function(b) if( !b ) log("Write failure"));
				}

				function sendBatch() {
					for( i in 0...1+Std.random(10) )
						send();
				}
				sendBatch();

				client.readStart(function(b) {
					totRB += b.length;
					for( i in 0...b.length ) {
						var k = b.get(i);
						if( !numbers.remove(k) )
							throw "!";
					}
					if( numbers.length == 0 ) {					
						if( Std.random(10000) == 0 ) {
							startClient();
							client.close();							
						} else							
							sendBatch();
					}
				});

			});

		}

		for( i in 0...4 )
			startClient();


		log("Enter Loop");

		var K = 0;
		var maxRead = 1000000;
		while( loop.run(NoWait) != 0 ) {
			if( K++ % 10000 == 0 ) log("Read=" + totR);

			if( totR > maxRead ) {
				log("Total Read > " + maxRead);
				break;
			}
		}

		log("Done");
		Sys.exit(0);
	}

}