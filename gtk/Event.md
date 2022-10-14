# 参考
1. [input-handling](https://docs.gtk.org/gtk3/input-handling.html)
# Event masks
> If a widget is windowless (gtk_widget_get_has_window() returns FALSE) and an application wants to receive custom events on it, it must be placed inside a GtkEventBox to receive the events, and an appropriate event mask must be set on the box. When implementing a widget, use a GDK_INPUT_ONLY GdkWindow to receive the events instead.

- 无窗口小部件（使用`gtk_widget_get_has_window()`为假判断）无法接收事件信号,可以使用`set_has_window (false)`来禁止输入， 想接收输入事件需要将其置于**GtkEventBox**内
  
2.在一个长时间的计算中更新UI
   ```vala
    while (Gtk.events_pending ()) {
        Gtk.main_iteration ();
    }

    //public bool main_iteration_do (bool blocking) 
   ```
