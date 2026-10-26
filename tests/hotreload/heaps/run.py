#!/usr/bin/env python3
"""
Hot reload test on a Heaps app.

The app (src/) runs with `hl --hot-reload` and is compiled with `-D hot-reload`: Heaps checks for a
reload every frame. For each step, the driver edits the sources of the app or of a copy of Heaps,
compiles through a compilation server, and waits for the status printed by the app (or for the
reload to be refused).

    python3 run.py --hl <hl> --haxe <haxe> --heaps <heaps> --format <format> [--hlsdl <libs/sdl>]

On Linux without a display, the app runs in xvfb-run. Heaps must be a version that compiles with the
compiler (haxe6-annotations branch of Heaps for haxe6-incremental).
"""
import argparse, os, queue, re, shutil, subprocess, sys, threading, time

HERE = os.path.dirname(os.path.abspath(__file__))

SHADER = """
class %s extends hxsl.Shader {
	static var SRC = {
		@param var factor : Float;
		var pixelColor : Vec4;
		function fragment() {
			pixelColor.rgb *= factor * %s;
		}
	}
}
"""

def game(version, body, extra=""):
	return """class Game {
	public static function init( app : Main ) {
		var t = new h2d.Text(hxd.res.DefaultFont.get(), app.s2d);
		t.text = "hello";
		new h2d.Bitmap(h2d.Tile.fromColor(0xFF0000, 16, 16), app.s2d);
	}
	public static function status( app : Main ) : String {
		%s
	}
}
%s
%s""" % (body.replace("VERSION", version), extra, "" if "class Tint " in extra else SHADER % ("Tint", "1."))

V0 = game("v0", 'return "VERSION objects=" + app.s2d.getObjectsCount();')
HELPER = """
class Helper {
	public static var calls : Int;
	public static function label() {
		calls++;
		return "helper";
	}
}
"""


# each step : the new Game.hx (None : unchanged), the replacements in Heaps (file, old, new),
# then what the app must print : a status, or a refusal (substring of the message)
STEPS = [
	dict(name="method body", game=game("v1", 'return "VERSION objects=" + app.s2d.getObjectsCount();'),
		status="v1 objects=3"),
	dict(name="Heaps method body", heaps=[("h2d/Object.hx", "k += c.getObjectsCount() + 1;\n\t\treturn k;", "k += c.getObjectsCount() + 1;\n\t\treturn k + 1000;")],
		status="v1 objects=4003"),
	dict(name="new class", game=game("v2", 'return "VERSION objects=" + app.s2d.getObjectsCount() + " " + Helper.label() + " " + (Helper.calls > 0);', HELPER),
		status="v2 objects=4003 helper true"),
	dict(name="Heaps inline functions", heaps=[
			("hxd/Math.hx", "return if( da > -max && da < max ) b else a + (da < 0 ? -max : max);", "return if( da > -max && da < max ) b * 2 else a + (da < 0 ? -max : max);"),
			("hxd/Math.hx", "public static inline function clamp( f : Float, min = 0., max = 1. ) {\n\t\treturn f < min ? min : f > max ? max : f;", "public static inline function clamp( f : Float, min = 0., max = 1. ) {\n\t\treturn f > max ? max : f < min ? min : f;"),
		],
		game=game("v3", 'return "VERSION objects=" + app.s2d.getObjectsCount() + " " + Helper.label() + " " + (Helper.calls > 0) + " move=" + hxd.Math.angleMove(0, 1, 2) + " clamp=" + hxd.Math.clamp(5);', HELPER),
		status="v3 objects=4003 helper true move=2 clamp=1"),
	dict(name="field added to a live Heaps class", heaps=[("h2d/Text.hx", "public var textColor(default, set) : Int;", "public var textColor(default, set) : Int;\n\tpublic var addedField : Int;")],
		game=game("v3b", 'return "VERSION objects=" + app.s2d.getObjectsCount() + " text=" + Std.downcast(app.s2d.getChildAt(0), h2d.Text).addedField;'),
		status="v3b objects=4003 text=0"),
	dict(name="field removed again", heaps=[("h2d/Text.hx", "public var textColor(default, set) : Int;\n\tpublic var addedField : Int;", "public var textColor(default, set) : Int;")],
		game=game("v4", 'return "VERSION objects=" + app.s2d.getObjectsCount();'),
		status="v4 objects=4003"),
	dict(name="Heaps method reverted", heaps=[("h2d/Object.hx", "return k + 1000;", "return k;")],
		status="v4 objects=3"),
	dict(name="statics added to a live Heaps class", heaps=[("h2d/Object.hx", "\tstatic var nullDrawable : h2d.Drawable;\n", "\tstatic var nullDrawable : h2d.Drawable;\n\tpublic static var reloadValue = 7;\n\tpublic static function reloadLabel() return \"label\" + reloadValue;\n")],
		game=game("v5", 'return "VERSION objects=" + app.s2d.getObjectsCount() + " " + h2d.Object.reloadLabel();'),
		status="v5 objects=3 label7"),
	dict(name="statics removed from a live Heaps class", heaps=[("h2d/Object.hx", "\tpublic static var reloadValue = 7;\n\tpublic static function reloadLabel() return \"label\" + reloadValue;\n", "")],
		game=game("v6", 'return "VERSION objects=" + app.s2d.getObjectsCount();'),
		status="v6 objects=3"),
	dict(name="new shader class",
		game=game("v7", 'var s = new Tint2(); s.factor = 0.5; return "VERSION objects=" + app.s2d.getObjectsCount() + " shader=" + s.factor;', SHADER % ("Tint2", "1.")),
		status="v7 objects=3 shader=0.5"),
	dict(name="shader source changed",
		game=game("v8", 'return "VERSION objects=" + app.s2d.getObjectsCount() + " changed=" + (hxsl.Printer.shaderToString(@:privateAccess new Tint().shader.data).indexOf("0.25") >= 0);', SHADER % ("Tint", "0.25")),
		status="v8 objects=3 changed=true"),
] + [
	dict(name="quick edit %d" % i, game=game("q%d" % i, 'return "VERSION objects=" + app.s2d.getObjectsCount();'), status="q%d objects=3" % i)
	for i in range(10)
]

class App:
	def __init__(self, cmd, cwd, env):
		self.proc = subprocess.Popen(cmd, cwd=cwd, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, bufsize=1)
		self.lines = queue.Queue()
		self.log = []
		threading.Thread(target=self.read, daemon=True).start()

	def read(self):
		for line in self.proc.stdout:
			line = line.rstrip("\r\n")
			self.log.append(line)
			self.lines.put((time.time(), line))
		self.lines.put((time.time(), None))

	def wait(self, pred, timeout):
		"""returns (time, line) of the first line matching pred, the lines read before it"""
		seen = []
		end = time.time() + timeout
		while True:
			try:
				t, line = self.lines.get(timeout=max(0.01, end - time.time()))
			except queue.Empty:
				return None, seen
			if line is None:
				return None, seen
			if pred(line):
				return (t, line), seen
			seen.append(line)

def main():
	p = argparse.ArgumentParser()
	p.add_argument("--hl", default="hl")
	p.add_argument("--haxe", default="haxe")
	p.add_argument("--heaps", required=True)
	p.add_argument("--format", required=True)
	p.add_argument("--hlsdl", default=os.path.join(HERE, "..", "..", "..", "libs", "sdl"))
	p.add_argument("--port", type=int, default=6790)
	p.add_argument("--timeout", type=float, default=60)
	p.add_argument("--gdb", action="store_true", help="run hl in gdb and print the native stack on a crash")
	a = p.parse_args()

	work = os.path.join(HERE, "..", "out", "heaps")
	shutil.rmtree(work, ignore_errors=True)
	shutil.copytree(os.path.join(HERE, "src"), os.path.join(work, "src"))
	shutil.copytree(a.heaps, os.path.join(work, "heaps"), ignore=shutil.ignore_patterns(".git", "samples"))
	# the changes of Heaps for hot reload (shaders), until Heaps has them
	with open(os.path.join(work, "heaps", "hxsl", "Macros.hx")) as f:
		patched = "a hot reload sets it again" in f.read()
	if not patched:
		# outside of any repository : the copy is inside the one of hashlink
		env = dict(os.environ, GIT_CEILING_DIRECTORIES=os.path.abspath(work))
		subprocess.run(["git", "apply", os.path.join(HERE, "heaps.patch")], cwd=os.path.join(work, "heaps"), env=env, check=True)
	for f in ("quit",):
		if os.path.exists(os.path.join(work, f)): os.remove(os.path.join(work, f))

	server = subprocess.Popen([a.haxe, "--wait", str(a.port)], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
	args = ["--cwd", work, "-cp", "src", "-cp", "heaps", "-cp", os.path.abspath(a.format), "-cp", os.path.abspath(a.hlsdl),
		"-D", "hlsdl=2.0.0", "-D", "heaps=2.1.0", "-D", "format=3.7.0", "-D", "hot-reload", "-dce", "no", "-D", "hl-ver=2.0.0",
		"-D", "hl-memo-check", "-main", "Main", "-hl", "app.hl"]

	def compile():
		t = time.time()
		for _ in range(50):
			r = subprocess.run([a.haxe, "--connect", str(a.port)] + args, capture_output=True, text=True)
			out = r.stdout + r.stderr
			if "Couldn't connect" not in out: break
			time.sleep(0.2)
		if r.returncode != 0 or "hl memo check" in out:
			raise Exception("compilation failed:\n" + out)
		return time.time() - t

	def edit(step):
		if step.get("game"):
			with open(os.path.join(work, "src", "Game.hx"), "w") as f: f.write(step["game"])
		for file, old, new in step.get("heaps", []):
			path = os.path.join(work, "heaps", file)
			with open(path, newline="") as f: s = f.read()
			crlf = "\r\n" in s
			s = s.replace("\r\n", "\n")
			if s.count(old) != 1: raise Exception("%s : %d occurrences of %r" % (file, s.count(old), old))
			s = s.replace(old, new)
			if crlf: s = s.replace("\n", "\r\n")
			with open(path, "w", newline="") as f: f.write(s)

	failures = 0
	app = None
	try:
		with open(os.path.join(work, "src", "Game.hx"), "w") as f: f.write(V0)
		ct = compile()
		print("initial compilation %.2fs" % ct)
		cmd = [a.hl, "--hot-reload", "app.hl"]
		if a.gdb:
			cmd = ["gdb", "-q", "-batch", "-ex", "handle SIGTRAP nostop noprint nopass", "-ex", "handle SIGPIPE nostop noprint pass", "-ex", "run", "-ex", "bt 30", "--args"] + cmd
		if sys.platform.startswith("linux") and not os.environ.get("DISPLAY"):
			cmd = ["xvfb-run", "-a", "-s", "-screen 0 1280x1024x24"] + cmd
		app = App(cmd, work, dict(os.environ))
		r, seen = app.wait(lambda l: l == "status v0 objects=3", a.timeout)
		if r is None: raise Exception("the app did not start:\n" + "\n".join(seen))

		for step in STEPS:
			edit(step)
			ct = compile()
			t0 = time.time()
			if "refused" in step:
				r, seen = app.wait(lambda l: "Reload refused" in l or l.startswith("status "), a.timeout)
				ok = r is not None and "Reload refused" in r[1]
				if ok:
					r2, seen2 = app.wait(lambda l: step["refused"] in l or l.startswith("[HotReload] Restart"), 5)
					ok = r2 is not None and step["refused"] in r2[1]
					seen += seen2
			else:
				r, seen = app.wait(lambda l: l.startswith("status ") or "Reload refused" in l, a.timeout)
				ok = r is not None and r[1] == "status " + step["status"]
			changes = [l for l in seen if re.match(r"\[HotReload\] \d+ changes", l)]
			latency = "" if r is None else " reload %.2fs" % (r[0] - t0)
			known = step.get("known")
			print("[%s] %s : compile %.2fs%s %s" % ("OK" if ok else "KNOWN" if known else "FAIL", step["name"], ct, latency, " ".join(changes)))
			if not ok and known:
				print("  " + known + ", got: " + (r[1] if r else "nothing"))
			elif not ok:
				failures += 1
				print("  expected %s, got %s" % (step.get("status") or "refused: " + step["refused"], r and r[1]))
				for l in seen: print("    " + l)
				if app.proc.poll() is not None:
					print("  the app stopped")
					break
	finally:
		if app:
			open(os.path.join(work, "quit"), "w").close()
			try:
				app.proc.wait(timeout=20)
			except subprocess.TimeoutExpired:
				app.proc.kill()
				print("the app did not quit")
				failures += 1
			with open(os.path.join(work, "app.log"), "w") as f: f.write("\n".join(app.log) + "\n")
			print("app exit code %s, log in %s" % (app.proc.returncode, os.path.join(work, "app.log")))
			code = app.proc.returncode
			if a.gdb and any("exited normally" in l for l in app.log): code = 0 # gdb fails on bt
			if code != 0: failures += 1
		server.kill()
	print("%d failures" % failures)
	sys.exit(1 if failures else 0)

main()
