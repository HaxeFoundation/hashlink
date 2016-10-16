#if (!js && !flash)
import haxe.io.Bytes;
#end

#if anon_objects
typedef RGB = { r:Int, g:Int, b:Int };
typedef Complex = { i:Float, j:Float };
#else
class RGB
{
   public var r:Int;
   public var g:Int;
   public var b:Int;

   public function new(inR:Int, inG:Int, inB:Int)
   {
      r = inR;
      g = inG;
      b = inB;
   }
}

class Complex
{
   public var i:Float;
   public var j:Float;

   public function new(inI:Float, inJ:Float)
   {
      i = inI;
      j = inJ;
   }
}
#end


@:result(18045783)
class Mandelbrot
{
   static inline var SIZE = 10;
   static inline var MaxIterations = 1000;
   static inline var MaxRad = 1<<16;
   static inline var width = 35*SIZE;
   static inline var height = 20*SIZE;


   public static function main()
   {
      var palette = [];
      for(i in 0...MaxIterations+1)
         palette.push( createPalette(i/MaxIterations) );

      var image = [];
      image[ width*height-1 ] = null;
      var outPixel = 0;
      var scale = 0.1/SIZE;
      for(y in 0...height)
      {
         for(x in 0...width)
         {
            var offset = createComplex(x*scale - 2.5,y*scale - 1);
            var val = createComplex(0.0,0.0);
            var iteration = 0;
            while( complexLength2(val)<MaxRad && iteration<MaxIterations)
            {
               val = complexAdd( complexSquare(val), offset );
               iteration++;
            }
            image[outPixel++] = palette[iteration];
         }
      }

	  var i = 0;
	  for( v in image )
		i += v.r + v.g + v.b;
	  Benchs.result(i);
   }

   public static function complexLength2(val:Complex) : Float
   {
      return val.i*val.i + val.j*val.j;
   }

   public static function complexAdd(val0:Complex, val1:Complex)
   {
      return createComplex( val0.i + val1.i, val0.j + val1.j );
   }

   public static function complexSquare(val:Complex)
   {
      return createComplex(  val.i*val.i - val.j*val.j, 2.0 * val.i * val.j );
   }


   #if anon_objects
   inline public static function createComplex(inI:Float, inJ:Float) return { i:inI, j:inJ };
   #else
   inline public static function createComplex(inI:Float, inJ:Float) return new Complex(inI, inJ);
   #end

   public static function createPalette(inFraction:Float)
   {
      var r = Std.int(inFraction*255);
      var g = Std.int((1-inFraction)*255);
      var b = Std.int( (0.5-Math.abs(inFraction-0.5))*2*255 );
      #if anon_objects
      return { r:r, g:g, b:b };
      #else
      return new RGB(r,g,b);
      #end
   }
}

