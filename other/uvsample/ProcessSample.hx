import hl.uv.Process;
import sys.thread.Thread;

class ProcessSample {
	public static function main() {
		// Process.disableStdioInheritance();

		var env = Sys.environment();
		env.set('DUMMY', '123');

		var cmd = switch Sys.systemName() {
			case 'Windows': 'dir';
			case _: 'ls';
		}

		var opt:ProcessOptions = {
			cwd: '..',
			stdio: [INHERIT, INHERIT, INHERIT],
			env: env,
			onExit: (p, exitCode, termSignal) -> {
				Log.print('process finished: exitCode $exitCode, termSignal $termSignal');
				p.close(() -> Log.print('process closed'));
			},
		}
		var p = Process.spawn(Thread.current().events, cmd, [cmd], opt);
		Log.print('pid ${p.pid}');
		// p.kill(SIGINT);
		// Process.killPid(p.pid, SIGINT);
	}
}