class UVSample {
	static function main() {
		run();
		// CheckSample.main();
		// PrepareSample.main();
		// IdleSample.main();
		// TcpSample.main();
		// DnsSample.main();
		// UdpSample.main();
		// PipeSample.main();
		// ProcessSample.main();
		// FileSample.main();
		// DirSample.main();
		// FsEventSample.main();
		// FsPollSample.main();
		// MiscSample.main();
		// TtySample.main();
		// SignalSample.main();
		// VersionSample.main();
	}

	macro static public function run() {
		var sample = haxe.macro.Context.definedValue('SAMPLE');
		if(sample == null || sample == '')
			sample = Sys.getEnv('SAMPLE');
		if(sample == null || sample == '') {
			Sys.println('Add -D SAMPLE=<sample> or set environment variable SAMPLE=<sample>');
			Sys.println('\twhere <sample> is a sample name. For example: SAMPLE=Tcp');
			Sys.exit(1);
		}
		sample += 'Sample';
		return macro $i{sample}.main();
	}
}