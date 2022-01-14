## Gdk.Pixbuf
A pixel buffer.
`GdkPixbuf` contains information about an image's pixel data, its color space, bits per sample, width and height, and the rowstride (the number of bytes between the start of one row and the start of the next)
- scale
- copy
- rotate
  
## Gtk.Image
- way 1
    ```vala
    Gtk.Image image = new Gtk.Image ();
    var thumb = new Gdk.Pixbuf (Gdk.Colorspace.RGB, false, 8, THUMB_WIDTH * scale, THUMB_HEIGHT * scale);
    image.gicon = thumb
    ```
- way 2
    ```vala
    Gtk.Image image = new Gtk.Image ();
    try {
        Gtk.IconTheme icon_theme = Gtk.IconTheme.get_default ();
        image.pixbuf = icon_theme.load_icon (icon_name, 16, 0);
    } catch (Error e) {
        warning (e.message);
    }
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
1. Granite.Drawing.BufferSurface-->blur
    ```vala
        private Gdk.Pixbuf load_file (string input) {
            Gdk.Pixbuf pixbuf = null;
            try {
                pixbuf = new Gdk.Pixbuf.from_file (input);
            } catch (Error e) {
                GLib.warning (e.message);
            }

            var surface = new Granite.Drawing.BufferSurface (pixbuf.width, pixbuf.height);
            Gdk.cairo_set_source_pixbuf (surface.context, pixbuf, 0, 0);
            surface.context.paint ();
            //  surface.exponential_blur (3);
            surface.gaussian_blur (3);
            surface.context.paint ();
            return Gdk.pixbuf_get_from_surface (surface.surface, 0, 0, pixbuf.width, pixbuf.height);
        }
    ```

## Actor设置图形
```vala
  Gdk.Pixbuf pixbuf = Gala.Utils.get_icon_for_window (window, icon_size, scale);
  try {
      var image = new Clutter.Image ();
      Cogl.PixelFormat pixel_format = (pixbuf.get_has_alpha () ? Cogl.PixelFormat.RGBA_8888 : Cogl.PixelFormat.RGB_888);
      image.set_data (pixbuf.get_pixels (), pixel_format, pixbuf.width, pixbuf.height, pixbuf.rowstride);
      Clutter.Actor actor = new Clutter.Actor()
      actor.set_content (image);
      actor.set_size(pixbuf.width, pixbuf.height);
  } catch (Error e) {
```
