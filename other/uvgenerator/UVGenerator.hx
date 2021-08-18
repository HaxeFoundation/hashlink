import haxe.Exception;
import sys.io.File;
import eval.luv.File.FileSync;
import haxe.io.Path;
import haxe.PosInfos;
import eval.luv.Dir.DirSync;

using StringTools;

typedef TypeAndName = {
	type:String,
	name:String
}

typedef FunctionSignature = {
	var name:String;
	var returnType:String;
	var arguments:Array<TypeAndName>;
}

class Skip extends Exception {}

class UVGenerator {
	static inline var DOCS = 'include/libuv/docs/src';
	static inline var UV_GENERATED_C = 'libs/uv/uv_generated.c';
	static inline var UV_HX = 'other/uvgenerator/UV.hx';
	static inline var UV_HX_HEADER = 'other/uvgenerator/UV.hx.header';

	static final skipDocs = ['api', 'dll', 'guide', 'index', 'migration_010_100',
		'poll', 'threading', 'threadpool', 'upgrading'];
	static final skipFunctions = ['uv_loop_configure']; // TODO: don't skip these
	static final allowNoCallback = ['uv_fs_cb'];

	static function main() {
		var root = rootDir();
		var docsDir = Path.join([root, DOCS]);
		var scan = DirSync.scan(docsDir).resolve();
		var cFile = File.write(Path.join([root, UV_GENERATED_C]));
		var hxFile = File.write(Path.join([root, UV_HX]));

		hxFile.writeString(File.getContent(Path.join([root, UV_HX_HEADER])));

		var entry = null;
		while(null != (entry = scan.next())) {
			if(entry.kind != FILE || '.rst' != entry.name.sub(entry.name.length - 4))
				continue;

			if(skipDocs.contains(entry.name.sub(0, entry.name.length - 4).toString()))
				continue;

			var path = Path.join([docsDir, entry.name.toString()]);
			Sys.println('Generating ' + entry.name.sub(0, entry.name.length - 4).toString() + '...');

			for(line in File.getContent(path).split('\n')) {
				if(!line.startsWith('.. c:function:: '))
					continue;
				try {
					var sig = try {
						parseSignature(line.substr('.. c:function:: '.length).trim());
					} catch(e:Skip) {
						continue;
					}
					var cStr = needsCbWrapper(sig) ? cWrapperWithCb(sig) : cBinding(sig);
					cFile.writeString(cStr);
					hxFile.writeString(hxBinding(sig));
				} catch(e) {
					Sys.stderr().writeString('Error on line: $line\n${e.details()}\n');
					Sys.exit(1);
				}
				cFile.writeString('\n');
			}
		}
		hxFile.writeString('}\n');

		cFile.close();
		hxFile.close();
		scan.end();
	}

	static function needsCbWrapper(sig:FunctionSignature):Bool {
		for(a in sig.arguments)
			if(a.type.endsWith('_cb')) {
				return true;
			}
		return false;
	}

	static function rootDir(?p:PosInfos):String {
		var generatorPath = FileSync.realPath(p.fileName).resolve().toString();
		return new Path(new Path(new Path(generatorPath).dir).dir).dir;
	}

	static function parseSignature(str:String):FunctionSignature {
		str = str.substr(0, str.length - (str.endsWith(';') ? 2 : 1));
		var parts = str.split('(');
		var returnAndName = parseTypeAndName(parts[0]);
		var args = parts[1].split(',').map(StringTools.trim);
		if(skipFunctions.contains(returnAndName.name))
			throw new Skip('${returnAndName.name} is in the skipFunctions list');
		return {
			name: returnAndName.name,
			returnType: returnAndName.type,
			arguments: args.map(parseTypeAndName)
		}
	}

	static function parseTypeAndName(str:String):TypeAndName {
		var namePos = str.lastIndexOf(' ') + 1;
		if(namePos <= 0)
			return {
				type: str,
				name: ''
			}
		else
			return {
				type: str.substring(0, namePos - 1),
				name: str.substr(namePos)
			}
	}

	static function mapHLType(type:String):String {
		return switch type {
			case 'int': '_I32';
			case 'int64_t': '_I64';
			case 'uint64_t': '_I64';
			case 'char*': '_BYTES';
			case 'void*': '_POINTER';
			case 'void': '_VOID';
			case 'unsigned int': '_U32';
			case 'ssize_t': '_I64';
			case 'size_t*': '_REF(_I64)';
			case 'size_t': '_U64';
			case 'int*': '_REF(_I32)';
			case 'double*': '_REF(_F64)';
			case 'double': '_F64';
			case 'FILE*': '_FILE';
			case 'uv_pid_t': '_I32';
			case 'uv_buf_t': '_BUF';
			case _ if(type.endsWith('**')):
				'_REF(' + mapHLType(type.substr(0, type.length - 1)) + ')';
			case _ if(type.startsWith('uv_')):
				type = type.substr(3);
				if(type.endsWith('_t*'))
					type = type.substring(0, type.length - 3);
				'_' + type.toUpperCase();
			case _ if(type.startsWith('struct ') && type.endsWith("*")):
				'_' + type.substring('struct '.length, type.length - 1).toUpperCase();
			case _ if(type.startsWith('const ')):
				mapHLType(type.substr('const '.length));
			case _:
				throw 'Unknown type: "$type"';
		}
	}

	static function mapHLArg(a:TypeAndName):String {
		var type = mapHLType(a.type);
		if(a.name.endsWith(']'))
			type = '_REF($type)';
		return type;
	}

	static final reCapitalize = ~/_[a-z]/g;

	static function snakeToPascalCase(str:String):String {
		return str.charAt(0).toUpperCase() + reCapitalize.map(str.substr(1), r -> r.matched(0).replace('_', '').toUpperCase());
	}

	static function mapHXType(type:String):String {
		if(type.startsWith('const '))
			type = type.substr('const '.length);

		return switch type {
			case 'void*': 'Pointer';
			case 'char*': 'Bytes';
			case 'double': 'Float';
			case 'int64_t': 'I64';
			case 'uint64_t': 'U64';
			case 'size_t': 'U64';
			case 'ssize_t': 'I64';
			case _ if(type.startsWith('unsigned ')):
				'U' + mapHXType(type.substr('unsigned '.length));
			case _ if(type.startsWith('struct ') && type.endsWith('*')):
				mapHXType(type.substring('struct '.length, type.length - 1));
			case _ if(type.startsWith('uv_') && type.endsWith('_t*')):
				type = type.substr(3, type.length - 3 - 3);
				snakeToPascalCase(type);
			case _ if(type.startsWith('uv_') && type.endsWith('_t')):
				type = type.substr(3, type.length - 3 - 2);
				snakeToPascalCase(type);
			case _ if(type.endsWith('*')):
				var hxType = mapHXType(type.substr(0, type.length - 1));
				'Ref<$hxType>';
			case _:
				snakeToPascalCase(type);
		}
	}

	static function functionName(name:String):String {
		return if(name.startsWith('uv_'))
			name.substr('uv_'.length)
		else
			throw 'Function name is expected to start with "uv_": "$name"';
	}

	static function functionNameWithCb(name:String):String {
		return '${functionName(name)}_with_cb';
	}

	static function callbackName(type:String):String {
		return 'on_${type}';
	}

	static function hxBinding(sig:FunctionSignature):String {
		function compose(name:String, args:Array<String>) {
			var args = args.join(', ');
			var ret = mapHXType(sig.returnType);
			return '\tstatic public function $name($args):$ret;\n';
		}
		function mapArg(a:TypeAndName):String {
			if(a.name.endsWith(']')) {
				var openPos = a.name.lastIndexOf('[');
				return '${a.name.substring(0, openPos)}:Ref<${mapHXType(a.type)}>';
			} else {
				return '${a.name}:${mapHXType(a.type)}';
			}
		}
		if(needsCbWrapper(sig)) {
			var fnName = functionNameWithCb(sig.name);
			var args = sig.arguments
				.filter(a -> !a.type.endsWith('_cb') || allowNoCallback.contains(a.type))
				.map(a -> a.type.endsWith('_cb') && allowNoCallback.contains(a.type) ? 'use_${a.type}:Bool' : mapArg(a));
			return compose(fnName, args);
		} else {
			var fnName = functionName(sig.name);
			var args = sig.arguments.filter(a -> a.type != 'void').map(mapArg);
			return compose(fnName, args);
		}
	}

	static function cBinding(sig:FunctionSignature):String {
		var args = switch sig.arguments {
			case [{type:'void'}]: '_NO_ARG';
			case _: sig.arguments.map(mapHLArg).join(' ');
		}
		return 'DEFINE_PRIM(${mapHLType(sig.returnType)}, ${functionName(sig.name)}, $args);\n';
	}

	static function cWrapperWithCb(sig:FunctionSignature):String {
		var fnName = functionNameWithCb(sig.name);

		var args = sig.arguments
			.filter(a -> !a.type.endsWith('_cb') || allowNoCallback.contains(a.type))
			.map(a -> a.type.endsWith('_cb') && allowNoCallback.contains(a.type) ? 'bool use_${a.type}' : '${a.type} ${a.name}')
			.join(', ');
		var lines = ['HL_PRIM ${sig.returnType} HL_NAME($fnName)( $args ) {'];

		var args = sig.arguments
			.map(a -> {
				var cbName = callbackName(a.type);
				if(a.type.endsWith('_cb'))
					allowNoCallback.contains(a.type) ? 'use_${a.type}?$cbName:NULL' : cbName
				else
					if(a.name.endsWith(']')) {
						var openPos = a.name.lastIndexOf('[');
						a.name.substring(0, openPos);
					} else
						a.name;
			})
			.join(', ');
		var ret = sig.returnType == 'void' ? '' : 'return ';
		lines.push('	$ret${sig.name}($args);');
		lines.push('}');

		var args = sig.arguments
			.filter(a -> !a.type.endsWith('_cb') || allowNoCallback.contains(a.type))
			.map(a -> a.type.endsWith('_cb') && allowNoCallback.contains(a.type) ? '_BOOL' : mapHLArg(a))
			.join(' ');
		lines.push('DEFINE_PRIM(${mapHLType(sig.returnType)}, $fnName, $args);');

		return lines.join('\n') + '\n';
	}
}

