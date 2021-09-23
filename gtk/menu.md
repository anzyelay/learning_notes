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