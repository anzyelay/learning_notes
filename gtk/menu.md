## popmenu
```vala
// 1. create menu item
var move_to_trash = new Gtk.MenuItem.with_label (_("Remove"));
move_to_trash.activate.connect (() => trash ());

// 2. add item to menu
context_menu = new Gtk.Menu ();
context_menu.append (move_to_trash);
context_menu.show_all ();

// 3. popup somewhere
context_menu.popup_at_pointer (null);
```
## comboxbox 
### model-view-read/write
```vala
    //create
    Gtk.CellRendererText renderer = new Gtk.CellRendererText ();// 设置GtkTreeModel中的每一项数据如何在列表框中显示
    var format_store = new Gtk.ListStore (2, typeof (string), typeof (string)); // model

    format_combobox = new Gtk.ComboBox.with_model (format_store);
    format_combobox.pack_start (renderer, true);
    format_combobox.add_attribute (renderer, "text", 0);
    format_combobox.changed.connect (on_format_changed);
    format_combobox.active = 0; // current item
    format_combobox.set_wrap_width(5) //下拉框一行有5列

    //write
    format_store.clear ();
    var iter = Gtk.TreeIter ();
    format_store.append (out iter);
    format_store.set (iter, 0, country, 1, locale);
    format_store.set_sort_column_id (0, Gtk.SortType.ASCENDING);

    //read
    Gtk.TreeIter iter;
    string format;
    if (!format_combobox.get_active_iter (out iter)) {
        return "";
    }
    format_store.get (iter, 1, out format);
    printf(format);

```