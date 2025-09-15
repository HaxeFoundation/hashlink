package hlmem;

#if js
enum FileSeekMode {
	SeekBegin;
	SeekCur;
	SeekEnd;
}
#else
typedef FileSeekMode = sys.io.FileSeek;
#end

class FileReader {

	#if js
	// js read file in once for better performance
	var memInput : haxe.io.BytesInput;
	#else
	var memInput : sys.io.FileInput;
	#end

	public inline function new(file : String) {
		#if js
		memInput = new haxe.io.BytesInput(sys.io.File.getBytes(file));
		#else
		memInput = sys.io.File.read(file);
		#end
	}

	public inline function close() {
		if( memInput != null )
			memInput.close();
		memInput = null;
	}

	public inline function readString(length : Int) : String {
		return memInput.readString(length);
	}

	public inline function readByte() : Int  {
		return memInput.readByte();
	}

	public inline function readInt32() : Int {
		return memInput.readInt32();
	}

	public inline function readPointer( is64 : Bool ) : haxe.Int64 {
		var low = readInt32();
		var high = is64 ? readInt32() : 0;
		return haxe.Int64.make(high,low);
	}

	public inline function tell() : Float {
		#if js
		return memInput.position;
		#else
		return tell2(@:privateAccess memInput.__f);
		#end
	}

	#if (hl && hl_ver >= version("1.12.0"))
	@:hlNative("std","file_seek2") static function seek2( f : sys.io.File.FileHandle, pos : Float, mode : Int ) : Bool { return false; }
	@:hlNative("std","file_tell2") static function tell2( f : sys.io.File.FileHandle ) : Float { return 0; }
	#end

	// pos will be cast to Int64
	public inline function seek( pos : Float, mode : FileSeekMode ) {
		#if js
		if( pos > 0x7FFFFFFF ) throw haxe.io.Error.Custom("seek out of bounds");
		var dpos = Std.int(pos);
		switch( mode ) {
		case SeekBegin:
			memInput.position = dpos;
		case SeekCur:
			memInput.position += dpos;
		case SeekEnd:
			memInput.position = memInput.length + dpos;
		}
		#else
		if( !seek2(@:privateAccess memInput.__f, pos, mode.getIndex()) )
			throw haxe.io.Error.Custom("seek2 failure()");
		#end
	}

}
