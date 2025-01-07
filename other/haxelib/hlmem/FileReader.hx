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
	var memBytes : haxe.io.Bytes;
	var memPos : Int;
	#else
	var memInput : sys.io.FileInput;
	#end

	public inline function new(file : String) {
		#if js
		memBytes = sys.io.File.getBytes(file);
		memPos = 0;
		#else
		memInput = sys.io.File.read(file);
		#end
	}

	public inline function close() {
		#if js
		memBytes = null;
		memPos = 0;
		#else
		if( memInput != null )
			memInput.close();
		memInput = null;
		#end
	}

	public inline function readString(length : Int) : String {
		#if js
		var str = memBytes.getString(memPos, 3);
		memPos += 3;
		#else
		var str = memInput.read(3).toString();
		#end
		return str;
	}

	public inline function readByte() : Int  {
		#if js
		var b = memBytes.get(memPos);
		memPos += 1;
		#else
		var b = memInput.readByte();
		#end
		return b;
	}

	public inline function readInt32() : Int {
		#if js
		var i = memBytes.getInt32(memPos);
		memPos += 4;
		#else
		var i = memInput.readInt32();
		#end
		return i;
	}

	public inline function readPointer( is64 : Bool ) : Block.Pointer {
		var low = readInt32();
		var high = is64 ? readInt32() : 0;
		return cast haxe.Int64.make(high,low);
	}

	public inline function tell() : Float {
		#if js
		return memPos;
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
			memPos = dpos;
		case SeekCur:
			memPos += dpos;
		case SeekEnd:
			memPos = memBytes.length + dpos;
		}
		#else
		if( !seek2(@:privateAccess memInput.__f, pos, mode.getIndex()) )
			throw haxe.io.Error.Custom("seek2 failure()");
		#end
	}

}
