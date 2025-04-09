function main() {
	var out = haxe.io.Bytes.alloc(16);
	var src = haxe.io.Bytes.ofString("Hello World!");
	hl.Format.digest(out.getData(), src.getData(), src.length, 0 /* md5 */);
	final expected = "ed076287532e86365e841e92bfc50d8c";
	final got = out.toHex();
	if (got != expected) {
		throw 'expected $expected, got $got';
	}
}