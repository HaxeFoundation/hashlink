import haxe.io.Bytes;
import hl.uv.UVError;
import haxe.Timer;
import hl.uv.UVException;
import sys.thread.Thread;
import hl.uv.Tcp;
import hl.uv.SockAddr;

class TcpSample {
	static inline var PORT = 22001;

	public static function main() {
		server();
		Timer.delay(client,100);
	}

	static function handle(success:()->Void):(e:UVError)->Void {
		return e -> switch e {
			case UV_NOERR: success();
			case _: throw new UVException(e);
		}
	}

	static function server() {
		inline function print(msg:String) {
			Log.print('SERVER: $msg');
		}
		var loop = Thread.current().events;
		var server = Tcp.init(loop, INET);
		server.bind(SockAddr.ipv4('0.0.0.0', PORT));
		server.listen(32, handle(() -> {
			var client = Tcp.init(loop);
			server.accept(client);
			print('connection from ' + client.getSockName());
			client.readStart((e, data, bytesRead) -> handle(() -> {
				print('incoming request: ' + data.toBytes(bytesRead).toString());
				client.write(data, bytesRead, handle(() -> {
					client.shutdown(handle(() -> {
						client.close(() -> print('client closed'));
					}));
				}));
			})(e));
		}));
	}

	static function client() {
		inline function print(msg:String) {
			Log.print('CLIENT: $msg');
		}
		var loop = Thread.current().events;
		var client = Tcp.init(loop, INET);
		client.connect(SockAddr.ipv4('127.0.0.1', PORT), handle(() -> {
			print('connected to ' + client.getPeerName());
			var data = Bytes.ofString('Hello, world!').getData();
			client.write(data.bytes, data.length, handle(() -> {
				client.readStart((e, data, bytesRead) -> handle(() -> {
					print('response from server: ' + data.toBytes(bytesRead).toString());
					client.shutdown(handle(() -> {
						client.close(() -> print('connection closed'));
					}));
				})(e));
			}));
		}));
	}
}