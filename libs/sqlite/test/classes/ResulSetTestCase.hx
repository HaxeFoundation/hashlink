package ;

import haxe.unit.TestCase;
import sys.FileSystem;

import sys.db.Connection;
import sys.db.Sqlite;
import sys.db.ResultSet;


class ResulSetTestCase extends TestCase
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
	
	public function testLength( ) : Void
	{
		var res : ResultSet = cnx.request('SELECT * FROM t1');
		assertFalse(res == null);
		assertTrue(res.hasNext());
		
		assertEquals(3, res.length);
	}
	public function testFieldsLen( ) : Void
	{
		var res : ResultSet = cnx.request('SELECT * FROM t1 WHERE i = 1');
		assertFalse(res == null);
		assertTrue(res.hasNext());
		
		assertEquals(5, res.nfields);
		var fields = res.getFieldsNames();
		assertFalse(fields == null);
		assertEquals(res.nfields, fields.length);
	}
	
	public function testFieldsName( ) : Void
	{
		var res : ResultSet = cnx.request('SELECT * FROM t1 WHERE i = 1');
		assertFalse(res == null);
		assertTrue(res.hasNext());
		
		var fields = res.getFieldsNames();
		
		assertEquals('i', fields[0]);
		assertEquals('f', fields[1]);
		assertEquals('t', fields[2]);
		assertEquals('bl', fields[3]);
		assertEquals('bo', fields[4]);
	}
	
	public function testFieldsVal1( ) : Void
	{
		var res : ResultSet = cnx.request('SELECT * FROM t1 WHERE i = 1');
		assertFalse(res == null);
		assertTrue(res.hasNext());
		
		var vals = res.next();
		
		assertEquals(1, vals.i);
		assertEquals(0.1, vals.f);
		assertEquals("hello", vals.t);
		assertEquals("\x11", vals.bl);
		assertEquals(true, vals.bo);
	}
	
	public function testFieldsVal2( ) : Void
	{
		var res : ResultSet = cnx.request('SELECT * FROM t1 WHERE i = 2');
		assertFalse(res == null);
		assertTrue(res.hasNext());
		
		var vals = res.next();
		
		assertEquals(2, vals.i);
		assertEquals(0.00002, vals.f);
		assertEquals("goodbye", vals.t);
		assertEquals("\x1111", vals.bl);
		assertEquals(false, vals.bo);
	}
	
	public function testFieldsVal3( ) : Void
	{
		var res : ResultSet = cnx.request('SELECT * FROM t1 WHERE i = 3');
		assertFalse(res == null);
		assertTrue(res.hasNext());
		
		var vals = res.next();
		
		assertEquals(3, vals.i);
		assertEquals(0.000000003, vals.f);
		assertEquals("Привет!", vals.t);
		assertEquals("\x111111", vals.bl);
		assertEquals(true, vals.bo);
	}
}