package;


class Main
{
	public static function main( ) : Void
	{
		var r = new haxe.unit.TestRunner();
		r.add(new BasicTestCase());
		r.add(new ResulSetTestCase());
		r.add(new ExceptionTestCase());
		r.run();
	}
	/*
	public static function main( ) : Void
	{
		var cnx : Connection = Sqlite.open('test.sqlite');
		cnx.close();
		
		if( FileSystem.exists('test2.sqlite') )
			FileSystem.deleteFile('test2.sqlite');
		
		var cnx : Connection = Sqlite.open('test2.sqlite');
		cnx.request('CREATE TABLE t1(a int, b text)');
		cnx.request('INSERT INTO t1 VALUES(1,"hello")');
		cnx.request('INSERT INTO t1 VALUES(2,"goodbye")');
		cnx.request('INSERT INTO t1 VALUES(3,"howdy!")');
		
		var res = cnx.request('SELECT a as column_name_a, b as field_name_b, * FROM t1 ORDER BY a');
		trace(res.getFieldsNames());
		for ( r in res )
			trace(r);
		
		cnx.close();
	}
	*/
}