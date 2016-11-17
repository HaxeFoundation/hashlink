package;

import haxe.unit.TestCase;
import sys.FileSystem;

import sys.db.Connection;
import sys.db.Sqlite;
import sys.db.ResultSet;


class BasicTestCase extends TestCase
{
	var file : String;
	var cnx : Connection;
	
	public function new( )
	{
		super();
		
		file = 'basic.sqlite';
	}
	
	public function testOpen( ) : Void
	{
		cnx = Sqlite.open(file);
		assertFalse(cnx == null);
		assertTrue(FileSystem.exists(file));
	}
	
	public function testDbName( ) : Void
	{
		assertEquals("SQLite", cnx.dbName());
	}
	
	public function testTransaction( ) : Void
	{
		cnx.startTransaction();
		cnx.commit();
		//cnx.startTransaction();
		cnx.commit();
		assertEquals(1, 1);
	}
	
	public function testSelect( ) : Void
	{
		var res : ResultSet = cnx.request("SELECT 1 as a");
		assertFalse(res == null);
		
		assertTrue(res.hasNext());
		var vals = res.next();
		assertFalse(vals == null);
		assertEquals(1, vals.a);
	}
	
	public function testClose( ) : Void
	{
		cnx.close();
		assertTrue(FileSystem.exists(file));
		FileSystem.deleteFile(file);
	}
}