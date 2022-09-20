## 1. 如何改窗口圆角？
原因： gtk的窗口如果不自行设置添加一个标题栏，窗口管理器会为它会自动添加一个[参考:gnome下标题栏样式的统一整理记录](./../blog/gnome下标题栏样式的统一整理记录.md)，此时即使用如下方式的css更改外框圆角时仍会出现黑色背景，现有的方案无法去掉这个黑色背景，因为它由窗口管理器添加，窗口外框的影响因素有两个：
- background类
- decoration
```xml
.background {
border-bottom-left-radius:10px;
border-bottom-right-radius:10px;
}

decoration {
border-bottom-left-radius:10px;
border-bottom-right-radius:10px;
}
```
解决方案：
1. 使用hdy.Window，也需要自己设置标题栏，但不能用set_titlebar,而是作用子控件加入。初始化时必须先调用hdy.init哦～～～
2. 使用gtk.window时，自行创建一个标题栏并使用set_titlebar方法加上，然后用上面的css调整圆角
    > `HdyWindow` 小部件是 [class@Gtk.Window] 的子类，它没有标题栏区域并在所有边提供圆角，确保它们永远不会被内容重叠。
