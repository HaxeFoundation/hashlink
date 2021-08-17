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
	static final skipDocs = ['api', 'dll', 'guide', 'index', 'migration_010_100',
		'poll', 'threading', 'threadpool', 'upgrading'];
	static final skipFunctions = ['uv_loop_configure'];
	static final allowNoCallback = ['uv_fs_cb'];

	static function main() {
		var root = rootDir();
		var uvGeneratedC = Path.join([root, 'libs', 'uv', 'uv_generated.c']);
		var docsDir = Path.join([root, 'include', 'libuv', 'docs', 'src']);
		var outFile = File.write(uvGeneratedC);
		var scan = DirSync.scan(docsDir).resolve();

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
					var needsWrapper = false;
					for(a in sig.arguments)
						if(a.type.endsWith('_cb')) {
							needsWrapper = true;
							break;
						}
					var str = needsWrapper ? generateWrapperWithCb(sig) : generateBinding(sig);
					outFile.writeString(str);
				} catch(e) {
					Sys.stderr().writeString('Error on line: $line\n${e.details()}\n');
					Sys.exit(1);
				}
				outFile.writeString('\n');
			}
		}

		outFile.close();
		scan.end();
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

	static function mapType(type:String):String {
		return switch type {
			case 'int': '_I32';
			case 'int64_t': '_I64';
			case 'uint64_t': '_I64';
			case 'char*': '_BYTES';
			case 'void*': '_DYN';
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
				'_REF(' + mapType(type.substr(0, type.length - 1)) + ')';
			case _ if(type.startsWith('uv_')):
				type = type.substr(3);
				if(type.endsWith('_t*'))
					type = type.substring(0, type.length - 3);
				'_' + type.toUpperCase();
			case _ if(type.startsWith('struct ')):
				type = '_' + type.substr('struct '.length).toUpperCase();
				type.endsWith('*') ? '_REF(${type.substr(0, type.length - 1)})' : type;
			case _ if(type.startsWith('const ')):
				mapType(type.substr('const '.length));
			case _:
				throw 'Unknown type: "$type"';
		}
	}

	static function functionName(name:String):String {
		return if(name.startsWith('uv_'))
			name.substr('uv_'.length)
		else
			throw 'Function name is expected to start with "uv_": "$name"';
	}

	static function callbackName(type:String):String {
		// if(type.startsWith('uv_'))
		// 	type = type.substr('uv_'.length);
		return 'on_${type}';
	}

	static function generateBinding(sig:FunctionSignature):String {
		var args = sig.arguments.map(a -> mapType(a.type)).join(' ');
		return 'DEFINE_PRIM(${mapType(sig.returnType)}, ${functionName(sig.name)}, $args);\n';
	}

	static function generateWrapperWithCb(sig:FunctionSignature):String {
		var fnName = '${functionName(sig.name)}_with_cb';

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
					a.name;
			})
			.join(', ');
		var ret = sig.returnType == 'void' ? '' : 'return ';
		lines.push('	$ret${sig.name}($args);');
		lines.push('}');

		var args = sig.arguments
			.filter(a -> !a.type.endsWith('_cb') || allowNoCallback.contains(a.type))
			.map(a -> a.type.endsWith('_cb') && allowNoCallback.contains(a.type) ? '_BOOL' : mapType(a.type))
			.join(' ');
		lines.push('DEFINE_PRIM(${mapType(sig.returnType)}, $fnName, $args);');

		return lines.join('\n') + '\n';
	}
}

