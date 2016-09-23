package;

import haxe.unit.TestCase;
import sys.FileSystem;
import sys.db.Connection;
import sys.db.Sqlite;

class OpenCloseTestCase extends TestCase
{
	var file : String;
	var cnx : Connection;
	
	override public function setup( ) : Void
	{
		super.setup();
		
		file = '${currentTest.classname}.sqlite';
	}
	
	public function testOpen( ) : Void
	{
		
		cnx = Sqlite.open(file);
	}
	
	public function testClose( ) : Void
	{
		cnx.close();
		assertTrue(FileSystem.exists(file));
	}
}