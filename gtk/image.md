## Gtk.Image
- way 1
```vala
thumb = new Gdk.Pixbuf (Gdk.Colorspace.RGB, false, 8, THUMB_WIDTH * scale, THUMB_HEIGHT * scale);
image.gicon = thumb
```
- way 2
```vala

```

## Granite.Asyncimage   
1. set_from_file_async()
1. set_from_gicon_async()
- way 1
```vala
try {
    yield image.set_from_file_async (File.new_for_path (thumb_path), THUMB_WIDTH, THUMB_HEIGHT, false);
} catch (Error e) {
    warning (e.message);
}
```