/*
Glib.application: 它封装了一些特定于平台的底层服务，旨在充当高层应用程序类（如 GtkApplication 或 MxApplication）的基础。  通常，您不应该在更高层级的框架之外使用此类。
1. 通过维护一个主应用实例(primary application instance)的‘use count’（使用计数）来提供一个更方便的生命周期管理服务。这个使用计数可以通过**hold()**和**release()**方法来增减操作，当计数为0时，应用退出,结束整个生命周期。
2. 通过一个唯一的application_id（应用id号）来标识一个应用，从而在一个session(图形登录会话界面)中保持该应用的进程唯一性。当一个相同id的应用再次被启动时，它的参数会通过相应平台的通信方式(linux下为D-bus session bus)传递给已经运行起来的拥有此id的应用，这个已经运行的应用就叫主实例(primary instance)

> There is a number of different entry points into a GApplication:
>   - via ‘Activate’ (i.e. just starting the application)
>   - via ‘Open’ (i.e. opening some files)
>   - by handling a command-line
>   - via activating an action


from: https://wiki.gnome.org/HowDoI/GtkApplication
from: https://valadoc.org/gio-2.0/GLib.Application.html
how to build: valac --pkg gio-2.0 main_args.vala
*/
public class MyApplication : Application {
	private int counter = 0;

	private MyApplication () {
		Object (application_id: "org.example.application", flags: ApplicationFlags.HANDLES_COMMAND_LINE);
		set_inactivity_timeout (10000);// 最后一次调用release()到inactivity之间的时间
	}
	//无参数启动时被调用，有参数启动时调用open方法
	public override void activate () {
		this.hold ();
		print ("Activated\n");
		this.release ();
	}


	private int _command_line (ApplicationCommandLine command_line) {
		bool version = false;
		bool count = false;

		OptionEntry[] options = new OptionEntry[2];
		options[0] = { "version", 'v', 0, OptionArg.NONE, ref version, "Display version number", null };
		options[1] = { "count", 'c', 0, OptionArg.NONE, ref count, "Display count number", null };


		// We have to make an extra copy of the array, since .parse assumes
		// that it can remove strings from the array without freeing them.
		string[] args = command_line.get_arguments ();
		string*[] _args = new string[args.length];
		for (int i = 0; i < args.length; i++) {
			_args[i] = args[i];
		}

		try {
			var opt_context = new OptionContext ("- OptionContext example");
			opt_context.set_help_enabled (true);
			opt_context.add_main_entries (options, null);
			unowned string[] tmp = _args;
			opt_context.parse (ref tmp);
		} catch (OptionError e) {
			command_line.print ("error: %s\n", e.message);
			command_line.print ("Run '%s --help' to see a full list of available command line options.\n", args[0]);
			return 0;
		}

		if (version) {
			command_line.print ("Test 0.1\n");
			return 0;
		}

		if (count) {
			command_line.print ("%d\n", ++this.counter);
			return 0;
		}


		return 0;
	}

	public override int command_line (ApplicationCommandLine command_line) {
		// keep the application running until we are done with this commandline
        print ("enter command_line ======\n");
		this.hold ();
		int res = _command_line (command_line);
		this.release ();

        //  activate ();

		return res;
	}

	public static int main (string[] args) {
		MyApplication app = new MyApplication ();
		int status = app.run (args);
		return status;
	}
}
