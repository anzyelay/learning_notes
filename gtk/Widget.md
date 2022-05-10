## Geometry Management 
1. 几何变化的三种形式
    - Height-for-width (高随宽度变化)
    - Width-for-Height (宽随高度变化)
    - Constant-size
    ```
        public enum SizeRequestMode {
            HEIGHT_FOR_WIDTH,       // Prefer height-for-width geometry management
            WIDTH_FOR_HEIGHT,       // Prefer width-for-height geometry management
            CONSTANT_SIZE           // Don’t trade height-for-width or width-for-height
        }
    ```

2. 设置widget的几何变化通过重载实现如下5个虚方法实现：
    ```c
    get_request_mode   // 重载它指定某种形式的变化
    get_preferred_width
    get_preferred_height
    get_preferred_height_for_width
    get_preferred_width_for_height
    get_preferred_height_and_baseline_for_width
    ```

    example
    - HEIGHT_FOR_WIDTH
    ```vala
        public override Gtk.SizeRequestMode get_request_mode () {
            return Gtk.SizeRequestMode.HEIGHT_FOR_WIDTH;
        }
        // 初始调用一次, 当m>width_request时，可以设置显示的实际最小值为m，
        public override void get_preferred_width (out int m, out int n) {
            m = n = 0;
        }
        //宽width变化时就会调用
        public override void get_preferred_height_for_width (int width, out int minimum_height, out int natural_height) {
            minimum_height = natural_height = (int)(width * ratio);
        }
    ```
  

## Gtk.FlowBox
1. 当一行可以显示所有childbox时， 不会自动调节大小，最后会空余出一部分空间。
2. 当一行正好可以显示所有或者不能显示所有childbox时，会自动调整childbox的大小适配宽度

