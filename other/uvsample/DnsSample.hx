import hl.uv.SockAddr;
import hl.uv.UVException;
import sys.thread.Thread;
import hl.uv.Dns;

class DnsSample {
	public static function main() {
		var loop = Thread.current().events;
		Dns.getAddrInfo(loop, 'haxe.org', 'http',
			{ flags: AI_CANONNAME, family: INET },
			(e, infos) -> switch e {
				case UV_NOERR:
					for(i in infos) {
						Log.print('getAddrInfo: addr ${i.addr}, canonname ${i.canonName}');
						if(i.canonName != null) {
							Dns.getNameInfo(loop, i.addr, NI_NAMEREQD,
								(e, name, service) -> switch e {
									case UV_NOERR:
										Log.print('getNameInfo: host $name, service $service');
									case _:
										throw new UVException(e);
								}
							);
						}
					}
				case _:
					throw new UVException(e);
			}
		);
	}
}