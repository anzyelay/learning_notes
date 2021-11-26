![inspector](../picture/gtk/inspector-miscellaneous.png)
## 开启调试功能
`gsettings set org.gtk.Settings.Debug enable-inspector-keybinding true`
## miscellaneous
```xml
H_______________________________________________I________M_________M'
|                                  |            :        |scroll.w :
|                             margin-top        :        |         :
|            (x,y) a_______________|____________b        |         :
|                  |          (allocation)      |        |         :
|            (clip | erea)                      |        |         :
|                  |                |           |        |         :
|                  |    <—w—>       h           | ——margin-end——   :
|                  |                |           |        |         :
| ——margin-start—— |                            |        |         :
|..................|____________________________|        |         :
|J                 d            |               c        |         :
|                               |                        |         :
|                            bottom                      |         :
|______________________________ | _______________________|         :
|K                              |                         L        : 
|                               |                                  :
|                               |                 (scroll erea)    :
|scroll.h                       |                                  :
|...............................|..................................:
K'                                                               L'

```
- allocation erea: 图中abcd矩形
- clip erea:k图中HIcJ矩形 
- parent window: HMLK矩形, L`要么与L重合，要么出来滚动窗

1. 分配区(选中控件后闪色的区块)
    - 定义： width x height + x-offset + y-offset (简写为：w*h+x+y)
    - 说明: 
      - x: 分配区左侧到父窗口的空隙间距(即 margin-star t值)
      - y: 分配区顶部到父窗口的空隙间距(即 margin-top值)
      - w: `p.w = margin.start + w + (margin.end - scroll.w)`; 选中分配区的宽，scroll.w=(parent.w-w-x)，要想不出现scrolledWindow水平滚动条，则需scroll.w=0
      - h: `p.h=margin.top + h + (margin.bottom-scroll.h)`;选中分配区的高，scroll.h=(parent.h-h-y)，要想不出现滚动条，需scroll.h=0。
    - 总结：分配宽+margin.start+margin.end 即为要在父窗口中分配显示的宽度,如果此值超过父窗口的宽度，当父窗口不固定则扩大宽度，如果固定则从margin.end边缘向左的部分显示区域被遮挡;高度亦如是。
2. 剪切区:
    - 定义： width x height + x-offset + y-offset (cw*ch+cx+cy)
    - 说明: 
      - cx: margin-start value
      - cy: margin-top value
      - cw:  cw + margin-end = parent.cw 
      - ch:  ch + margin-bottom = parent.ch
3. 分配区和剪切区的关系：
   - 满足：cw=w+x, ch=h+y
   - example
        ```note
        - 无滚动情况：
        parent-allo: 704x575+0+0
        parent-clip: 704x575+0+0

        child-allo: 634x465+30+50
        child-clip: 664x515+0+0
        child-margin: start:30,end:40,top:50,bottom:60

        allo: 704=634+30+40   575=465+50+60
        clip: 704=664+40      575=515+60

        - 有上下滚动条的情况：（高度不够）
        parent-allo:704x465+0+0
        parent-clip:704x465+0+0

        child-allo:672x404+16+32
        child-clip:688x436+0+0
        child-margin: start:16,end:16,top:32,bottom:32

        allo: 704=672+16+16     465=404+32+29
        clip: 704=688+16        465=436+29
        因为29<32，所以垂直方向出现滚动条，把margin-bottom调整为29即无  
        ```

## css
selector![css selector](../picture/gtk/inspector-css-selector.png)
css node![css node](../picture/gtk/inspector-css-node.png)
- scss code :
  ```scss
  levebar {
      &.discrete {
          &.horizontal {
              block {
                  min-width: rem(10px);
              }
          }
  }
  ```
- inspector css :
  ```css
  levelbar.discrete.horizontal block {
    min-width:10px;
  }
  #101 {
    min-width:24px;
    border:2px solid red;
  }

  ```
- #ID 选择某个控件指定，效果如下图, 其余的block width=10
  ![id selector](../picture/gtk/inspector-css-example.png)
  
- 样式类别： 图中的样式类别就代表着当前所用的类名classname （eg: .horizontal, .discrete）
- 状态： 代表当前控件的状态是disable,active,backdrop等，状态以`classname:status`书写css
