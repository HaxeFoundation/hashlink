package dx;

@:noCompletion
typedef GameControllerPtr = hl.Abstract<"dx_gctrl_device">;

private class DInputButton {
	var num : Int;
	var mask : Int;
	var axis : Int;
	
	public function new( num : Int, mask : Int, axis : Int ){
		this.num = num;
		this.mask = mask;
		this.axis = axis;
	}
}

private class DInputMapping {
	var guid : Int;
	var name : hl.Bytes;
	var button : hl.NativeArray<DInputButton>;
	
	function new( s : String ){
		var a = s.split(",");
		var suid = a.shift();
		var vendor = Std.parseInt( "0x" + suid.substr(8,4) );
		var product = Std.parseInt( "0x" + suid.substr(16,4) );
		guid = (product&0xFF)<<24 | (product&0xFF00)<<8 | (vendor&0xFF)<<8 | (vendor&0xFF00)>>8;
		name = @:privateAccess a.shift().toUtf8();
		button = new hl.NativeArray(20);
		
		for( e in a ){
			var p = e.split(":");
			if( p.length != 2 ) continue;
			
			var btn : DInputButton = switch( p[1].charCodeAt(0) ){
				case 'b'.code: new DInputButton(Std.parseInt(p[1].substr(1)), 0, -1);
				case 'h'.code: {
					var ba = p[1].substr(1).split(".");
					if( ba == null ) 
						null;
					else
						new DInputButton(Std.parseInt(ba[0]), Std.parseInt(ba[1]), -1);
				}
				case 'a'.code:
					new DInputButton(-1,-1,Std.parseInt(p[1].substr(1)));
				default: null;
			}
			if( btn == null ) continue;

			switch( p[0] ){
				case "leftx":        button[0] = btn;
				case "lefty":        button[1] = btn;
				case "rightx":       button[2] = btn;
				case "righty":       button[3] = btn;
				case "lefttrigger":  button[4] = btn;
				case "righttrigger": button[5] = btn;
				default:
					var idx = switch( p[0] ){
						case "dpup":          Btn_DPadUp;
						case "dpdown":        Btn_DPadDown;
						case "dpleft":        Btn_DPadLeft;
						case "dpright":       Btn_DPadRight;
						case "start":         Btn_Start;
						case "back":          Btn_Back;
						case "leftstick":     Btn_LeftStick;
						case "rightstick":    Btn_RightStick;
						case "leftshoulder":  Btn_LB;
						case "rightshoulder": Btn_RB;
						case "a":             Btn_A;
						case "b":             Btn_B;
						case "x":             Btn_X;
						case "y":             Btn_Y;
						default: null;
					}
					if( idx != null )
						button[6+Type.enumIndex(idx)] = btn;
			}
		}
	}
	
	// Default DInput mappings based on libSDL ( https://www.libsdl.org/ )
	static var DEFAULTS = [
		"03000000022000000090000000000000,8Bitdo NES30 Pro,a:b1,b:b0,back:b10,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,leftshoulder:b6,leftstick:b13,lefttrigger:b8,leftx:a0,lefty:a1,rightshoulder:b7,rightstick:b14,righttrigger:b9,rightx:a3,righty:a4,start:b11,x:b4,y:b3,",
		"03000000203800000900000000000000,8Bitdo NES30 Pro,a:b1,b:b0,back:b10,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,leftshoulder:b6,leftstick:b13,lefttrigger:b8,leftx:a0,lefty:a1,rightshoulder:b7,rightstick:b14,righttrigger:b9,rightx:a3,righty:a4,start:b11,x:b4,y:b3,",
		"03000000102800000900000000000000,8Bitdo SFC30 GamePad,a:b1,b:b0,back:b10,leftshoulder:b6,leftx:a0,lefty:a1,rightshoulder:b7,start:b11,x:b4,y:b3,",
		"03000000a00500003232000000000000,8Bitdo Zero GamePad,a:b0,b:b1,back:b10,dpdown:+a2,dpleft:-a0,dpright:+a0,dpup:-a2,leftshoulder:b6,rightshoulder:b7,start:b11,x:b3,y:b4,",
		"03000000341a00003608000000000000,Afterglow PS3 Controller,a:b1,b:b2,back:b8,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,guide:b12,leftshoulder:b4,leftstick:b10,lefttrigger:b6,leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b11,righttrigger:b7,rightx:a2,righty:a3,start:b9,x:b0,y:b3,",
		"03000000e82000006058000000000000,Cideko AK08b,a:b2,b:b1,back:b8,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,leftshoulder:b4,leftstick:b10,lefttrigger:b6,leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b11,righttrigger:b7,rightx:a2,righty:a3,start:b9,x:b3,y:b0,",
		"03000000260900008888000000000000,Cyber Gadget GameCube Controller,a:b0,b:b1,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,lefttrigger:a5,leftx:a0,lefty:a1,rightshoulder:b6,righttrigger:a4,rightx:a2,righty:a3~,start:b7,x:b2,y:b3,",
		"03000000a306000022f6000000000000,Cyborg V.3 Rumble Pad,a:b1,b:b2,back:b8,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,leftshoulder:b4,leftstick:b10,lefttrigger:+a3,leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b11,righttrigger:-a3,rightx:a2,righty:a4,start:b9,x:b0,y:b3,",
		"03000000ffff00000000000000000000,GameStop Gamepad,a:b0,b:b1,back:b8,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,guide:,leftshoulder:b4,leftstick:b10,lefttrigger:b6,leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b11,righttrigger:b7,rightx:a2,righty:a3,start:b9,x:b2,y:b3,",
		"030000000d0f00006e00000000000000,HORIPAD 4 (PS3),a:b1,b:b2,back:b8,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,guide:b12,leftshoulder:b4,leftstick:b10,lefttrigger:b6,leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b11,righttrigger:b7,rightx:a2,righty:a3,start:b9,x:b0,y:b3,",
		"030000000d0f00006600000000000000,HORIPAD 4 (PS4),a:b1,b:b2,back:b8,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,guide:b12,leftshoulder:b4,leftstick:b10,lefttrigger:a3,leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b11,righttrigger:a4,rightx:a2,righty:a5,start:b9,x:b0,y:b3,",
		"030000000d0f00005f00000000000000,Hori Fighting Commander 4 (PS3),a:b1,b:b2,back:b8,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,guide:b12,leftshoulder:b4,lefttrigger:b6,leftx:a0,lefty:a1,rightshoulder:b5,righttrigger:b7,rightx:a2,righty:a3,start:b9,x:b0,y:b3,",
		"030000000d0f00005e00000000000000,Hori Fighting Commander 4 (PS4),a:b1,b:b2,back:b8,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,guide:b12,leftshoulder:b4,lefttrigger:a3,leftx:a0,lefty:a1,rightshoulder:b5,righttrigger:a4,rightx:a2,righty:a5,start:b9,x:b0,y:b3,",
		"030000008f0e00001330000000000000,HuiJia SNES Controller,a:b2,b:b1,back:b8,dpdown:+a1,dpleft:-a0,dpright:+a0,dpup:-a1,leftshoulder:b6,rightshoulder:b7,start:b9,x:b3,y:b0,",
		"030000006d04000016c2000000000000,Logitech Dual Action,a:b1,b:b2,back:b8,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,leftshoulder:b4,leftstick:b10,lefttrigger:b6,leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b11,righttrigger:b7,rightx:a2,righty:a3,start:b9,x:b0,y:b3,",
		"030000006d04000018c2000000000000,Logitech F510 Gamepad,a:b1,b:b2,back:b8,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,leftshoulder:b4,leftstick:b10,lefttrigger:b6,leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b11,righttrigger:b7,rightx:a2,righty:a3,start:b9,x:b0,y:b3,",
		"030000006d04000019c2000000000000,Logitech F710 Gamepad,a:b1,b:b2,back:b8,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,leftshoulder:b4,leftstick:b10,lefttrigger:b6,leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b11,righttrigger:b7,rightx:a2,righty:a3,start:b9,x:b0,y:b3,", /* Guide button doesn't seem to be sent in DInput mode. */
		"03000000380700005032000000000000,Mad Catz FightPad PRO (PS3),a:b1,b:b2,back:b8,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,guide:b12,leftshoulder:b4,leftstick:b10,lefttrigger:b6,leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b11,righttrigger:b7,rightx:a2,righty:a3,start:b9,x:b0,y:b3,",
		"03000000380700005082000000000000,Mad Catz FightPad PRO (PS4),a:b1,b:b2,back:b8,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,guide:b12,leftshoulder:b4,leftstick:b10,lefttrigger:a3,leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b11,righttrigger:a4,rightx:a2,righty:a5,start:b9,x:b0,y:b3,",
		"03000000790000004418000000000000,Mayflash GameCube Controller,a:b1,b:b2,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,lefttrigger:a3,leftx:a0,lefty:a1,rightshoulder:b7,righttrigger:a4,rightx:a5,righty:a2,start:b9,x:b0,y:b3,",
		"030000001008000001e5000000000000,NEXT SNES Controller,a:b2,b:b1,back:b8,dpdown:+a1,dpleft:-a0,dpright:+a0,dpup:-a1,leftshoulder:b4,rightshoulder:b6,start:b9,x:b3,y:b0,",
		"030000007e0500000920000000000000,Nintendo Switch Pro Controller,a:b0,b:b1,back:b8,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,guide:b12,leftshoulder:b4,leftstick:b10,lefttrigger:b6,leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b11,righttrigger:b7,rightx:a3,righty:a4,start:b9,x:b2,y:b3,",
		"03000000362800000100000000000000,OUYA Game Controller,a:b0,b:b3,dpdown:b9,dpleft:b10,dpright:b11,dpup:b8,guide:b14,leftshoulder:b4,leftstick:b6,lefttrigger:a2,leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b7,righttrigger:b13,rightx:a3,righty:a4,x:b1,y:b2,",
		"03000000888800000803000000000000,PS3 Controller,a:b2,b:b1,back:b8,dpdown:h0.8,dpleft:h0.4,dpright:h0.2,dpup:h0.1,guide:b12,leftshoulder:b4,leftstick:b9,lefttrigger:b6,leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b10,righttrigger:b7,rightx:a3,righty:a4,start:b11,x:b0,y:b3,",
		"030000004c0500006802000000000000,PS3 Controller,a:b14,b:b13,back:b0,dpdown:b6,dpleft:b7,dpright:b5,dpup:b4,guide:b16,leftshoulder:b10,leftstick:b1,lefttrigger:b8,leftx:a0,lefty:a1,rightshoulder:b11,rightstick:b2,righttrigger:b9,rightx:a2,righty:a3,start:b3,x:b15,y:b12,",
		"03000000250900000500000000000000,PS3 DualShock,a:b2,b:b1,back:b9,dpdown:h0.8,dpleft:h0.4,dpright:h0.2,dpup:h0.1,guide:,leftshoulder:b6,leftstick:b10,lefttrigger:b4,leftx:a0,lefty:a1,rightshoulder:b7,rightstick:b11,righttrigger:b5,rightx:a2,righty:a3,start:b8,x:b0,y:b3,",
		"030000004c050000c405000000000000,PS4 Controller,a:b1,b:b2,back:b8,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,guide:b12,leftshoulder:b4,leftstick:b10,lefttrigger:a3,leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b11,righttrigger:a4,rightx:a2,righty:a5,start:b9,x:b0,y:b3,", // DualShock 4 v1 (Wired or Wireless)
		"030000004c050000cc09000000000000,PS4 Controller,a:b1,b:b2,back:b8,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,guide:b12,leftshoulder:b4,leftstick:b10,lefttrigger:a3,leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b11,righttrigger:a4,rightx:a2,righty:a5,start:b9,x:b0,y:b3,", // DualShock 4 v2 (Wired or Wireless)
		"030000004c050000a00b000000000000,PS4 Controller,a:b1,b:b2,back:b8,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,guide:b12,leftshoulder:b4,leftstick:b10,lefttrigger:a3,leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b11,righttrigger:a4,rightx:a2,righty:a5,start:b9,x:b0,y:b3,", // DualShock 4 (Wireless with Sony adapter)
		"03000000790000001100000000000000,Retrolink SNES Controller,a:b2,b:b1,back:b8,dpdown:+a4,dpleft:-a3,dpright:+a3,dpup:-a4,leftshoulder:b4,rightshoulder:b5,start:b9,x:b3,y:b0,",
		"030000006b140000010d000000000000,Revolution Pro Controller,a:b1,b:b2,back:b8,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,guide:b12,leftshoulder:b4,leftstick:b10,lefttrigger:a3,leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b11,righttrigger:a4,rightx:a2,righty:a5,start:b9,x:b0,y:b3,",
		"03000000a30600000cff000000000000,Saitek P2500 Force Rumble Pad,a:b2,b:b3,back:b11,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,guide:b10,leftshoulder:b4,leftstick:b8,lefttrigger:b6,leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b9,righttrigger:b7,rightx:a2,righty:a3,x:b0,y:b1,",
		"03000000172700004431000000000000,XiaoMi Game Controller,a:b0,b:b1,back:b10,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,guide:b20,leftshoulder:b6,leftstick:b13,lefttrigger:b8,leftx:a0,lefty:a1,rightshoulder:b7,rightstick:b14,righttrigger:a7,rightx:a2,righty:a5,start:b11,x:b3,y:b4,",
		"03000000830500006020000000000000,iBuffalo SNES Controller,a:b1,b:b0,back:b6,dpdown:+a1,dpleft:-a0,dpright:+a0,dpup:-a1,leftshoulder:b4,rightshoulder:b5,start:b7,x:b3,y:b2,",
	];
	
	public static function parseDefaults() : hl.NativeArray<DInputMapping> {
		var a = [for( s in DEFAULTS ) new DInputMapping(s)];
		var n = new hl.NativeArray(a.length);
		for( i in 0...a.length ) n[i] = a[i];
		return n;
	}
}

enum GameControllerButton {
	Btn_DPadUp;
	Btn_DPadDown;
	Btn_DPadLeft;
	Btn_DPadRight;
	Btn_Start;
	Btn_Back;
	Btn_LeftStick;
	Btn_RightStick;
	Btn_LB;
	Btn_RB;
	Btn_A;
	Btn_B;
	Btn_X;
	Btn_Y;
}

@:keep @:hlNative("directx")
class GameController {
	public static var CONFIG = {
		analogX : 14,
		analogY : 15,
		ranalogX : 16,
		ranalogY : 17,
		LT : 18,
		RT : 19,

		dpadUp : 0,
		dpadDown : 1,
		dpadLeft : 2,
		dpadRight : 3,
		start : 4,
		back : 5,
		analogClick : 6,
		ranalogClick : 7,
		LB : 8,
		RB : 9,
		A : 10,
		B : 11,
		X : 12,
		Y : 13,

		names : ["DUp","DDown","DLeft","DRight","Start","Back","LCLK","RCLK","LB","RB","A","B","X","Y","LX","LY","RX","RY","LT","RT"],
	};

	public static var NUM_BUTTONS = 14;
	public static var NUM_AXES = 6;
	
	var ptr(default,null) : GameControllerPtr;
	public var index : Int;
	public var name(default,null) : String;
	public var buttons : haxe.EnumFlags<GameControllerButton>;
	public var axes : hl.Bytes;
	var rumbleEnd : Null<Float>;
	
	function new(){
	}
	
	public inline function update(){
		gctrlUpdate(this);
		if( rumbleEnd != null && haxe.Timer.stamp() > rumbleEnd ){
			gctrlSetVibration(ptr,0.);
			rumbleEnd = null;
		}
	}
	
	public inline function rumble( strength : Float, time_s : Float ){
		gctrlSetVibration(ptr,strength);
		rumbleEnd = strength <= 0 ? null : haxe.Timer.stamp() + time_s;
	}

	public inline function getAxis( i : Int ) {
		return (i<0 || i>=NUM_AXES) ? 0. : axes.getF64(i*8);
	}

	public inline function getButtons(){
		return buttons.toInt();
	}
	
	// 

	static var UID = 0;
	static var ALL = [];

	public static function init(){
		var mappings = DInputMapping.parseDefaults();
		gctrlInit( mappings );
	}
	
	public static function detect( onDetect : GameController -> Bool -> Void ){
		gctrlDetect(function(ptr:GameControllerPtr, name:hl.Bytes){
			if( name != null ){
				var d = new GameController();
				d.ptr = ptr;
				d.name = @:privateAccess String.fromUTF8(name);
				d.index = UID++;
				d.axes = new hl.Bytes(NUM_AXES*8);
				d.update();
				ALL.push(d);
				onDetect(d,true);
			}else{
				for( d in ALL ){
					if( d.ptr == ptr ){
						ALL.remove(d);
						onDetect(d,false);
						break;
					}
				}
			}
		});
	}
	
	static function gctrlInit( mappings : hl.NativeArray<DInputMapping> ){}
	static function gctrlDetect( onDetect : GameControllerPtr -> hl.Bytes -> Void ){}
	static function gctrlUpdate( pad : GameController ){}
	static function gctrlSetVibration( ptr : GameControllerPtr, strength : Float ){}

	
}
