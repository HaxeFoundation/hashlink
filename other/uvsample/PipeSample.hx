import sys.FileSystem;
import hl.uv.Misc;
import hl.uv.SockAddr;
import hl.uv.Tcp;
import haxe.io.Bytes;
import hl.uv.Pipe;
import hl.uv.UVException;
import hl.uv.UVError;
import haxe.PosInfos;
import sys.thread.Thread;
import haxe.Timer;

class PipeSample {
	static var NAME = 'testpipe';

	static public function main() {
		NAME = Misc.tmpDir() + '/' + NAME;
		#if CLIENT
		Log.print('Running PipeSample client...');
		client();
		#else
		Log.print('Running PipeSample server...');
		Log.print('waiting for connections');
		server();
		#end
	}

	static function handle(success:()->Void, ?p:PosInfos):(e:UVError)->Void {
		return e -> switch e {
			case UV_NOERR: success();
			case _: throw new UVException(e, p.fileName + ':' + p.lineNumber + ': ' + e.toString());
		}
	}

	static function server() {
		if(FileSystem.exists(NAME))
			FileSystem.deleteFile(NAME);
		function print(msg:String)
			Log.print('SERVER: $msg');
		var loop = Thread.current().events;
		var server = Pipe.init(loop);
		server.bind(NAME);
		server.listen(32, handle(() -> {
			var client = Pipe.init(loop, true);
			server.accept(client);
			print('connection from ' + client.getSockName());
			var tcp = Tcp.init(loop, INET);
			client.readStart((e, data, bytesRead) -> switch e {
				case UV_NOERR:
					print('incoming request: ' + data.toBytes(bytesRead).toString());
					var addr = Ip4Addr('93.184.216.34', 80); //http://example.com
					tcp.connect(addr, handle(() -> {
						print('tcp connected to ' + addr);
						client.write2(data, bytesRead, tcp, handle(() -> print('tcp sent')));
					}));
				case UV_EOF:
					print('client disconnected');
					tcp.close(() -> {
						print('tcp closed');
						client.close(() -> {
							print('pipe connection closed');
							server.close(() -> print('done'));
						});
					});
				case _:
					throw new UVException(e);
			});
		}));
	}

	static function client() {
		function print(msg:String)
			Log.print('CLIENT: $msg');
		var loop = Thread.current().events;
		var client = Pipe.init(loop, true);
		client.connect(NAME, handle(() -> {
			print('connected to ' + client.getPeerName());
			var data = Bytes.ofString('Hello, world!').getData();
			client.write(data.bytes, data.length, handle(() -> {
				var tcp = Tcp.init(loop);
				client.readStart((e, data, bytesRead) -> switch e {
					case UV_NOERR:
						while(client.pendingCount() > 0) {
							switch client.pendingType() {
								case UV_TCP:
									client.accept(tcp);
									print('Received tcp socket connected to ' + tcp.getPeerName());
								case _:
									throw 'Received unexpected handler type';
							}
						}
						print('response from server: ' + data.toBytes(bytesRead).toString());
						client.close(() -> print('pipe connection closed'));
					case UV_EOF:
						print('disconnected from server');
						client.close(() -> print('pipe connection closed'));
					case _:
						throw new UVException(e);
				});
			}));
		}));
	}
}