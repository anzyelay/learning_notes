class Funk : Object {
  public signal void funky();
  public virtual void funk() {
    funky();
  }
}

[DBus (name = "org.gnome.TestServer", signals = "funky, echo", properties = "name, mess, count")]
class TestServer: Funk {
  public string name {get; construct;}
  public string mess {get; set;}
  public int count {get; set;}

  public TestServer(string name) {
     Object (name : name);
      Timeout.add (1000, ()=>{
        count += 1;
        return true;
      });
      this.notify["count"].connect (()=>{
        echo (count.to_string ());
      });

  }
  construct {
     mess = name;
     count = 0;
  }
  public signal void echo(string msg);
  public override void funk() {
    base.funk();
  }
  public string ping(string msg) {
    message(msg);
    echo(mess);
    mess = msg;
    return mess;
  }
}
public class TestDemo :Object {
  public TestDemo () {
     Bus.own_name (BusType.SESSION, "org.gnome.TestServer", BusNameOwnerFlags.NONE,
                  on_bus_acquired, on_name_acquired, on_name_lost);
  }
  private void on_bus_acquired (DBusConnection conn) {
    print ("bus acquired\n");
    try {
        TestServer server = new TestServer("funky server");
        conn.register_object("/org/gnome/TestServer", server);
    } catch (IOError e) {
        print ("%s\n", e.message);
    }
  }
  private void on_name_acquired () {
      print ("name acquired\n");
  }  

  private void on_name_lost () {
      print ("name_lost\n");
  }

  public static void main(string[] args){
     MainLoop loop = new MainLoop();
     var demo = new TestDemo ();
    //   .... Setup dbus connection....
     loop.run();
     return;
  }
}
