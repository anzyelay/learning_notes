## Gdk.Pixbuf
A pixel buffer.
`GdkPixbuf` contains information about an image's pixel data, its color space, bits per sample, width and height, and the rowstride (the number of bytes between the start of one row and the start of the next)
- scale
    - 目标： 尽最大可能按目标窗体大小显示源图中最多的内容， 然后宽高等比例放大填满目标窗体。
    ```vala
    // 先整体放大图像，再平移剪切对应大小到目标上去。
    public Gdk.Pixbuf get_pixbuf_for_size (string path, int width_request, int height_request) {
        int dst_w = width_request;
        int dst_h = height_request;
        int width, height;
        Gdk.Pixbuf pixbuf = new Gdk.Pixbuf (Gdk.Colorspace.RGB, true, 8, dst_w, dst_h);

        Gdk.Pixbuf.get_file_info (path, out width, out height);
        double scale_x = (double)dst_w/width;
        double scale_y = (double)dst_h/height;
        var scale = double.max (scale_x, scale_y);
        double offset_x = (dst_w - width*scale)/2; // x轴平移距离   实际是: -(width*scale-dst_w)/2
        double offset_y = (dst_h - height*scale)/2;// y轴平移距离   实际同上
        try {
            Gdk.Pixbuf src_pixbuf = new Gdk.Pixbuf.from_file (path);
            //src_pixbuf先按scale缩放后，再平移offset距离，再将处理过的图形的目标区域渲染到目标图形上去
            src_pixbuf.scale (pixbuf, 0, 0, dst_w, dst_h, offset_x, offset_y, scale, scale, Gdk.InterpType.NEAREST);
        //  warning ("wxh:%dx%d, offset: %fx%f , %d,%d, src wxh:%dx%d", dst_w, dst_h, offset_x, offset_y
        //              ,(int)(scale_x*width), (int)(scale_y*height)
        //              ,width, height);
        } catch (Error e) {
            critical (e.message);
        }       
        return pixbuf;
    }
    // 与上面的方法处理结果相同
    // 先按目标宽高比裁剪源图，然后缩放图形到目标大小
    public Gdk.Pixbuf ?get_pixbuf_for_size (string path, int width_request, int height_request) {
            int dst_w = width_request;
            int dst_h = height_request;
            int width, height;
            Gdk.Pixbuf ?pixbuf = null;

            Gdk.Pixbuf.get_file_info (path, out width, out height);
            double scale_x = (double)dst_w/width;
            double scale_y = (double)dst_h/height;
            var scale = double.max (scale_x, scale_y);
            dst_w = (int)(dst_w/scale);
            dst_h = (int)(dst_h/scale);
            int src_x = (width-dst_w)/2;
            int src_y = (height-dst_h)/2;
            
            try {
                Gdk.Pixbuf src_pixbuf = new Gdk.Pixbuf.from_file (path);
                pixbuf = new Gdk.Pixbuf.subpixbuf (src_pixbuf, src_x, src_y, dst_w, dst_h);
                pixbuf = pixbuf.scale_simple (width_request, height_request, Gdk.InterpType.BILINEAR);
            } catch (Error e) {
                critical (e.message);
            }       
            return pixbuf;
    }
    ```
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

- 注意：
  - image控件的button_press_event不会触发，如果需要可使用DrawingArea

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
## DrawingArea 缩放图片
```vala
    public class BannerImage : Gtk.DrawingArea {
        private Cairo.ImageSurface? imgsurface = null;
        private int orig_width;
        private int orig_height;
        private double orig_ratio;
        
        public BannerImage(string png_path) {
            imgsurface = new Cairo.ImageSurface.from_png(png_path);
            orig_width = imgsurface.get_width();
            orig_height = imgsurface.get_height();
            orig_ratio = (double)orig_height / orig_width;
        }

        public override bool draw (Cairo.Context ctx) {
            if (imgsurface != null) {
                var width = get_allocated_width ();
                double sx = (double)width / orig_width;
                //  double sy = (double)frm.height / orig_h_;
                ctx.scale(sx, sx);
                ctx.set_source_surface(imgsurface, 0, 0);
                ctx.paint();
            }
            return true;
        }
    }
```
针对jpg等图的处理
```vala
    public class BannerImage : Gtk.DrawingArea {

        private Gdk.Pixbuf ?pixbuf = null;
        private int orig_width;
        private int orig_height;
        
        public BannerImage(string png_path) {
            try {
                pixbuf = new Gdk.Pixbuf.from_file (path);
            }catch (Error e) {
                warning (e.message);
            }
            orig_width = pixbuf.get_width ();
            orig_height = pixbuf.get_height ();
        }
        // override widget.draw
        public override bool draw (Cairo.Context ctx) {
            if (pixbuf != null) {
                var width = get_allocated_width ();
                var height = get_allocated_height ();

                double scale_x = (double)width/orig_width;
                double scale_y = (double)height/orig_height;
                var scale = double.max (scale_x, scale_y);
                double offset_x = (width/scale - orig_width)/2; // x轴平移距离   实际是: -(width*scale-dst_w)/2
                double offset_y = (height/scale - orig_height)/2;// y轴平移距离   实际同上

                ctx.scale(scale, scale);
                Gdk.cairo_set_source_pixbuf (ctx, pixbuf, offset_x, offset_y);
                ctx.paint();
                return true;
            }
            return false;
        }
    }
```
