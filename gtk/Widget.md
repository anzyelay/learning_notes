## 几个属性值
- expand: 有v和h两个方向， 指示是否占用父项给予的多余空间
- align: 有v和h两个方向， 指示在expand为true下如何占用空间, 默认值为FILL
## Geometry Management 
参见[Size allocation](https://wiki.gnome.org/HowDoI/CustomWidgets#What_virtual_functions_to_implement.3F)部分

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
    注： Bin类型的widget通常传播其孩子的偏好，容器类widget需要在其孩子的上下文中或在其分配能力的上下文中请求某些东西。故在设置容器时要注意align之类的值。

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

3. size allocate，分配给子项的大小规律如下
    - homogeneous为true:
        - FILL情况下，分配值为父size/子项个数==average_size()
        - 非FILL情况下， 取所有子项中的最大natural_size与上面的average_size取较小的一个当分配值。
    - homogeneous为false:
        - gtk_distribute_natural_allocation

## 可见性
name | 定义 | 适用范围 | 区别 
-|-|-|-
is_visible | 确定此widget及其所有父项是否标记为可见， 不检查小部件是否以任何方式被遮挡  | 所有 |   本身及其父项可见性
get_visible | 确定此widget是否能见，不检查小部件是否以任何方式被遮挡 |所有 | 本身可见性, set_visible设置该属性
set_visible | 设置widget的能见性状态，并不意味着此widget就实际可见, 此函数仅简单调用show或hide，但在小部件的可见性取决于某些条件时用此方法更好。| 所有 | 是否最终可见还要看父项的能见性状态
get_child_visible | 获取由set_child_visible设置的值 | 此函数仅对容器实现有用，绝不应由应用程序调用。 其它情况下如果你觉得有使用它的必要，可能你的代码需要重新梳理下 |  子项可见性
set_child_visible | 设置当它已经被调用show显示时，它是否跟随其父级一起映射 | 只在容器实现中 |  1. 可以在加入容器前设置，以避免在需要立即取消映射它们时不必要的映射子级；<br> 2. 从容器中被删时恢复默认值（true)； <br> 3. 更改widget的子可见性,widget不会排队调整大小。 大多数情况下，**widget的大小是根据所有可见子项计算得出的，无论它们是否被映射**。 如果不是这种情况，容器可以自己排队调整大小。
no_show_all | 该属性确定对 show_all 的调用是否会影响此小部件。 |   若为真，则调用show_all时不会有响应，直接调用visible为真则显示该部件，但子部件是否显示要看子部件情况。

## Gtk.FlowBox
1. 当一行可以显示所有childbox时， 不会自动调节大小，最后会空余出一部分空间。
2. 当一行正好可以显示所有或者不能显示所有childbox时，会自动调整childbox的大小适配宽度

