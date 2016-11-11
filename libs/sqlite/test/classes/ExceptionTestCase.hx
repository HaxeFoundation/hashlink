import haxe.unit.TestCase;
import sys.FileSystem;

import sys.db.Connection;
import sys.db.Sqlite;
import sys.db.ResultSet;


class ExceptionTestCase extends TestCase
{
	var file : String;
	var cnx : Connection;
	
	override public function setup( ) : Void
	{
		super.setup();
		
		file = '${currentTest.classname}-${currentTest.method}.sqlite';
		cnx = Sqlite.open(file);
		
		cnx.request('DROP TABLE IF EXISTS t1');
		cnx.request('CREATE TABLE t1(i int, f float, t text, bl blob, bo bool)');
		
		cnx.request('INSERT INTO  t1 VALUES(1, 0.1, "hello", "\x11", 1)');
		cnx.request('INSERT INTO  t1 VALUES(2, 0.00002, "goodbye", "\x1111", 0)');
		cnx.request('INSERT INTO  t1 VALUES(3, 0.000000003, "Привет!", "\x111111", 1)');
	}
	
	override public function tearDown( ) : Void
	{
		super.tearDown();
		
		cnx.close();
		FileSystem.deleteFile(file);
	}
	
	public function testSqlError( ) : Void
	{
		var sql : String = 'SELECT * FRM t1';
		var expected : String = 'SQLite error: near "FRM": syntax error';
		
		var msg : String = null;
		try
		{
			var res : ResultSet = cnx.request(sql);
		}
		catch ( e : Dynamic )
		{
			msg = Std.string(e);
		}
		
		assertEquals(expected, msg);
	}
	
	public function testSameField( ) : Void
	{
		var sql : String = 'SELECT i as a, f as a FROM t1';
		var expected : String = 'SQLite error: Same field is two times in the request: $sql';
		
		var msg : String = null;
		try
		{
			var res : ResultSet = cnx.request(sql);
		}
		catch ( e : Dynamic )
		{
			msg = Std.string(e);
		}
		
		assertEquals(expected, msg);
	}
}