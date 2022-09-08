## 需求：希望能向上拓展，且文字居中对齐：
 1. 如何改变子窗口的位置？   
无法更改
 2. 如何改变箭头的位置？  
使用文字方向设置，只能更改文字的位置，且只能居右或居右，无法居中，
解决方案： 自定义一个expander控件
## 自定义一个expander控件
```vala
public class Widgets.Expander: Gtk.Box {
    private Gtk.Revealer _revealer;
    private Gtk.Button _expand_btn;
    private Gtk.Image _arrow;
    private Gtk.Box _expand_title_box;
    //指示是向下拓展子窗口还是向上
    private bool _expand_to_down = true; 
    public bool expand_to_down { 
        get {
            return _expand_to_down;
        }
        set {
            if ( _expand_to_down == value ) {
                return;
            }
            _expand_to_down = value;
            reorder_child (_revealer, (value ? 1 : 0));
            if (_expanded) {
                if (value) {
                    _arrow.icon_name = "pan-down-symbolic"; 
                } 
                else {
                    _arrow.icon_name = "pan-up-symbolic";
                }
            }
            show_all ();
        } 
    }
    // 提示文字
    public string label {
        get {
            return _expand_btn.label;
        }
        set {
            _expand_btn.label = value;
        }
    }
    // 是否拓展开
    public bool _expanded = false;
    public bool expanded  { 
        get { return _expanded; } 
        set {
            if (_expanded == value) {
                return;
            }
            _expanded = value;
            _revealer.reveal_child = value;
            vexpand = value;
            if (value) {
                if (_expand_to_down) {
                    _arrow.icon_name = "pan-down-symbolic"; 
                } 
                else {
                    _arrow.icon_name = "pan-up-symbolic";
                }
            }
            else {
                _arrow.icon_name = "pan-end-symbolic";
            }
        } 
    }
    // 提示文字的对齐方式
    public Gtk.Align label_align {
        get { return _expand_title_box.halign; }
        set { _expand_title_box.halign = value; }
    }

    construct {
        orientation = Gtk.Orientation.VERTICAL;
        vexpand_set = true;
        _expand_btn = new Gtk.Button.with_label ("expander");
        _revealer = new Gtk.Revealer () {
            hexpand = true,
        };
        _arrow = new Gtk.Image.from_icon_name ("pan-end-symbolic",
                                                 Gtk.IconSize.MENU);

        _expand_title_box = new Gtk.Box (Gtk.Orientation.HORIZONTAL, 0);
        _expand_title_box.add (_expand_btn);
        _expand_title_box.add (_arrow);

        pack_start (_expand_title_box, false, false);
        pack_start (_revealer);

        GlobalCssProvider.get_default ().apply_2s ( {_expand_btn, _arrow} );
        _expand_btn.get_style_context ().add_class ("custom-link");
        _arrow.get_style_context ().add_class ("custom-link");

        _expand_btn.clicked.connect (()=>{
            expanded = !expanded; 
        });

    }

    public Expander (string text, bool expand2down = true) {
        label = text;
        label_align = Gtk.Align.CENTER;
        expand_to_down = expand2down;
        expanded = false;
    }

    public override void add (Gtk.Widget child) {
        if (_revealer.get_child () != null) {
            _revealer.remove (_revealer.get_child ());
        }
        _revealer.add (child);
    }
}

```
