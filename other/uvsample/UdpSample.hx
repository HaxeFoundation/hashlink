import haxe.Timer;
import haxe.io.Bytes;
import hl.uv.UVException;
import hl.uv.SockAddr;
import hl.uv.Udp;
import sys.thread.Thread;

class UdpSample {
	static inline var PORT = 22002;

	public static function main() {
		Log.print('Running UdpSample...');
		server();
		Timer.delay(client,100);
	}

	static function server() {
		function print(msg:String) {
			Log.print('RECEIVER: $msg');
		}
		var loop = Thread.current().events;
		var udp = Udp.init(loop, INET,true);
		udp.bind(Ip4Addr('0.0.0.0', PORT));
		var cnt = 0;
		udp.recvStart((e, data, bytesRead, addr, flags) -> switch e {
			case UV_NOERR:
				var msg = data.toBytes(bytesRead).toString();
				if(bytesRead > 0) {
					print('Received message from $addr: $msg');
				} else {
					print('recv callback invocation with addr = $addr');
					if(addr == null && !flags.mmsgChunk)
						udp.close(() -> print('Done'));
				}
				var o = {
					mmsgChunk: flags.mmsgChunk,
					mmsgFree: flags.mmsgFree,
					partial: flags.partial,
				}
				print('...with flags $o');
			case _:
				throw new UVException(e);
		});
	}

	static function client() {
		function print(msg:String) {
			Log.print('SENDER: $msg');
		}
		var udp = Udp.init(Thread.current().events, INET);
		var data = Bytes.ofString('Hello, UDP!');
		udp.send(data.getData(), data.length, Ip4Addr('127.0.0.1', PORT), e -> switch e {
			case UV_NOERR:
				print('Message sent');
				udp.close(() -> print('Done'));
			case _:
				throw new UVException(e);
		});
	}
}