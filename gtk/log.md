GLib.Log 针对g_warning, g_info, g_error..... 重定向的方式：
```vala
        GLib.Log.set_default_handler ((log_domain, log_levels, msg)=>{
            print ("domain:%s, %d, msg:%s\n", log_domain, log_levels, msg);
        });
```
