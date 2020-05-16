package clip;

@:hlNative("clipboard")
class Clipboard {
    static function get_clipboard_text () : hl.Bytes {
        return null;
    }

    public static function getText () {
        return @:privateAccess String.fromUTF8(get_clipboard_text());
    }
    
    static function set_clipboard_text (text : hl.Bytes) : Bool {
        return false;
    }

    public static function setText (text : String) {
        return @:privateAccess set_clipboard_text(text.toUtf8());
    }
}