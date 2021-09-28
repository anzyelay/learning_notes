- [dbus 信号的监听](#dbus-信号的监听)
- [dbus 方法的引用](#dbus-方法的引用)
- [dbus 属性的监听](#dbus-属性的监听)
- [直接使用](#直接使用)
## dbus 信号的监听
- 申明
    ```vala
    [DBus (name = "org.freedesktop.login1.Manager")]
    interface Manager : Object {
        public signal void prepare_for_sleep (bool sleeping);
    }
    ```
- 创建实例
    ```vala
    Manager manager = Bus.get_proxy_sync (BusType.SYSTEM, "org.freedesktop.login1", "/org/freedesktop/login1");
    ```
- 使用
    ```vala 
    // Listen for the signal that is fired when waking up from sleep, then update time
    manager.prepare_for_sleep.connect ((sleeping) => {
        if (!sleeping) {
            update_current_time ();
            minute_changed ();
            add_timeout ();
        }
    });

    ```
## dbus 方法的引用
- 申明
  ```vala
    [DBus (name = "org.freedesktop.Accounts")]
    interface FDO.Accounts : Object {
        public abstract string find_user_by_name (string username) throws GLib.Error;
    }
  ```
- 实例化
    ```vala
    var accounts_service = yield GLib.Bus.get_proxy<FDO.Accounts> (GLib.BusType.SYSTEM,
                                                                    "org.freedesktop.Accounts",
                                                                    "/org/freedesktop/Accounts");
    ```
- 使用
    ```vala
    var user_path = accounts_service.find_user_by_name (GLib.Environment.get_user_name ());
    ```

## dbus 属性的监听
- 申明
    ```vala
    [DBus (name = "io.elementary.pantheon.AccountsService")]
    interface Pantheon.AccountsService : Object {
        public abstract string time_format { owned get; set; }
    }
    ```
- 实例化
  ```vala
  var greeter_act = yield GLib.Bus.get_proxy<Pantheon.AccountsService> (GLib.BusType.SYSTEM,
                                            "org.freedesktop.Accounts",
                                            user_path,
                                            GLib.DBusProxyFlags.GET_INVALIDATED_PROPERTIES);
  ```

- 使用
    ```vala
    ((GLib.DBusProxy) greeter_act).g_properties_changed.connect ((changed_properties, invalidated_properties) => {
        if (changed_properties.lookup_value ("TimeFormat", GLib.VariantType.STRING) != null) {
            bool is_12h = ("12h" in greeter_act.time_format);
        }
    });
    ``` 

## 直接使用
```vala
private void on_watch (DBusConnection conn) {
    // Start do something because someone is changing settings
}

private void on_unwatch (DBusConnection conn) {
    // Stop doing 
}

// Listen for the D-BUS server that controls time settings
Bus.watch_name (BusType.SYSTEM, "org.freedesktop.timedate1", BusNameWatcherFlags.NONE, on_watch, on_unwatch);
```