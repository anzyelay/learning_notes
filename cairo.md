# [Cairo's Drawing Model](https://www.cairographics.org/tutorial/)

## 涉及到的几个概念

### 名词概念

1. Destination（目标） ：将要绘制到的平面（**surface**）,此平面可以绑定到一个二维像素矩阵(ImageSurface),PDF(PdfSurface),Svg(SvgSurface)等
   - 从PNG创建： `ImageSurface.from_png (string filename)`
   - 保存成PNG：`Surface.writo_to_png (string filename)`
2. Source（源）
   - 你用来绘图的颜料（paint）,与真实颜料不同的是，它可以不只是单一的颜色，也可以是图样（pattern)，surface，还能有透明通道元素
3. Mask
   - 遮罩是最重要的部分：它控制将源应用到目标的位置。当您应用绘图动作时，就像您将源(source))邮印（stamp）到目标(destination)。 在遮罩允许的任何地方复制源，在不允许的地方，什么都不会发生 
4. Path
   - 路径介于部分遮罩和部分上下文之间, 它由路径动作操纵，然后由绘制动作使用   
5. Context（绘制上下文）
   - 上下文跟踪绘图动作影响的一切。它跟踪一个源、一个目标和一个遮罩。它还跟踪多个辅助变量，例如线宽和样式、字体和大小等。最重要的是，它跟踪路径，通过绘制动作将其变成遮罩。
   - 创建：`Context (Surface target)`，
   - `save`,`restore`用来保存和恢复当前的上下文环境，比如字体，线宽，合成操作OVER/SOURCE等
   - 任何一个操作动作都必须先准备一个context

### 操作动作

1. Stroke: mask允许线路为path的source绘制到destination上   --need path
1. Fill: mask允许通过边界为path的洞，将source填充到destination上 -- need path
1. Show Text: text_path+fill操作     -- need 坐标位置
1. Paint: mask允许整个source转移到destination上 -- need primary source
1. Mask: 以一个pattern或surface为mask的遮罩操作source到destination -- need second pattern/surface

### summary:

所谓的操作动作其实就是更改mask让source如何绘制到destination，path就是mask上可穿透的路线

```xml
source ---> mask(path) ---> destination
```

## cairo绘图

1. 创建目标surface
1. 为surface创建一个context
1. 设置source
1. 构造好mask(path,pattern,surface)
1. 绘制操作

    ```vala
    public static int main (string[] args) {
        // Create a context:
        Cairo.ImageSurface surface = new Cairo.ImageSurface (Cairo.Format.ARGB32, 256, 256);
        Cairo.Context context = new Cairo.Context (surface);

        // The arc:
        context.arc (128.0, 128.0, 76.8, 0, 2*Math.PI);
        context.clip ();
        context.new_path (); // path not consumed by clip()

        // The image:
        string image_path = Path.build_filename (Path.get_dirname (args[0]) , "romedalen.png");
        Cairo.ImageSurface image = new Cairo.ImageSurface.from_png (image_path);
        int w = image.get_width ();
        int h = image.get_height ();
        context.scale (256.0/w, 256.0/h);

        context.set_source_surface (image, 0, 0);
        context.paint ();

        // Save the image:
        surface.write_to_png ("img.png");

        return 0;
    }
    ```

note:关于clear一个surface,cairo的默认合成操作是**OVER**,将一个完全透明的色混合后无任何效果，更改合成操作为**SOURCE**可替换原来色
