import hl.uv.Misc;

class MiscSample {
	public static function main() {
		Log.print('Resident set memory: ' + Misc.residentSetMemory());
		Log.print('Uptime: ' + Misc.uptime());
		Log.print('RUsage: ' + Misc.getRUsage());
		Log.print('Pid: ' + Misc.getPid());
		Log.print('PPid: ' + Misc.getPPid());
		Log.print('Cpu infos:\n  ' + Misc.cpuInfo().map(Std.string).join('\n  '));
		Log.print('Inteface addresses:\n  ' + Misc.interfaceAddresses().map(i -> {
			Std.string({
				name:i.name,
				physAddr:i.physAddr,
				isInternal:i.isInternal,
				address:i.address.name(),
				netmask:i.netmask.name(),
			});
		}).join('\n  '));
		Log.print('Load avg: ' + Misc.loadAvg());
		Log.print('Home dir: ' + Misc.homeDir());
		Log.print('Passwd: ' + Misc.getPasswd());
	}
}