import hl.Bytes;
import hl.uv.UVException;
import sys.thread.Thread;
import hl.uv.Misc;

class MiscSample {
	static function print(msg:String) {
		Log.print('MiscSample: $msg');
	}

	public static function main() {
		print('Resident set memory: ' + Misc.residentSetMemory());
		print('Uptime: ' + Misc.uptime());
		print('RUsage: ' + Misc.getRUsage());
		print('Pid: ' + Misc.getPid());
		print('PPid: ' + Misc.getPPid());
		print('Cpu infos:\n  ' + Misc.cpuInfo().map(Std.string).join('\n  '));
		print('Inteface addresses:\n  ' + Misc.interfaceAddresses().map(i -> {
			Std.string({
				name:i.name,
				physAddr:i.physAddr,
				isInternal:i.isInternal,
				address:i.address.name(),
				netmask:i.netmask.name(),
			});
		}).join('\n  '));
		print('Load avg: ' + Misc.loadAvg());
		print('Home dir: ' + Misc.homeDir());
		print('Passwd: ' + Misc.getPasswd());
		print('Free mem: ' + Misc.getFreeMemory());
		print('Total mem: ' + Misc.getTotalMemory());
		print('Constrained mem: ' + Misc.getConstrainedMemory());
		print('HR time: ' + Misc.hrTime());
		print('Host name: ' + Misc.getHostName());
		var myPid = Misc.getPid();
		Misc.setPriority(myPid, Misc.getPriority(myPid) + 1);
		print('Priority: ' + Misc.getPriority(myPid));
		print('Uname: ' + Misc.uname());
		print('Time of day: ' + Misc.getTimeOfDay());
		var buf = new Bytes(20);
		Misc.random(Thread.current().events, buf, 20, 0, e -> switch e {
			case UV_NOERR:
				print('Random bytes hex: ' + haxe.io.Bytes.ofData(new haxe.io.BytesData(buf, 20)).toHex());
			case _:
				throw new UVException(e);
		});
	}
}