# QT NOTES

## lambada

1. 传参说明

- 使用`&`号传的是引用， 如果传指针，可能外面的指针已删，此时继续使用引用值会发生段错误
  - 可以用`QPointer<type> ptr = new type()`来弱指针来处理，这样可以在内部判断其是否已被销毁，因为QPointer会自行将销毁的指针赋`nullptr`,经测试还是不行，推荐使用=传值吧。
- 使用`=`号传的是值, 不存在引用的段错误。

1. code分析

    ```c++
    QPointer<QMessageBox> msgBox = new QMessageBox(this);
    msgBox->setText("face test");
    msgBox->setInformativeText(tr("are u %1").arg(user.name()));
    QTimer::singleShot(1000, this, [&](){
        if (msgBox)
            delete msgBox;
        inner = false;
    });
    msgBox->exec();
    ```

    > 在你的代码中，存在一个典型的 悬空指针（Dangling Pointer） 问题。具体来说，QTimer::singleShot 中的 Lambda 表达式捕获了 msgBox 的引用（[&]），并在 1 秒后尝试删除 msgBox。然而，msgBox->exec() 是一个阻塞调用，它会一直运行直到用户关闭对话框。在此期间，如果 QTimer 触发了删除操作，msgBox 会被删除，但 exec() 仍在运行，导致访问已删除的对象，从而引发段错误（Segmentation Fault）。

    解决方法：使用智能指针（推荐）

    ```c++
    QSharedPointer<QMessageBox> msgBox(new QMessageBox(this));
    msgBox->setText("face test");
    msgBox->setInformativeText(tr("are u %1").arg(user.name()));

    // 使用 QTimer 关闭对话框
    QTimer::singleShot(1000, this, [msgBox]() {
        if (msgBox) {
            msgBox->close();  // 关闭对话框
        }
    });

    msgBox->exec();  // 阻塞直到用户关闭对话框
    ```

    1. 使用 `msgBox->close()` 关闭对话框，而不是直接删除 `msgBox`
    1. 在 `exec()` 结束后再删除 `msgBox`
    1. 使用 `QSharedPointer` 自动管理对象生命周期，无需手动删除。

## 控件介绍

### QRubberBand

A rubber band is often used to show a new bounding area (as in a QSplitter or a QDockWidget that is undocking). Historically this has been implemented using a QPainter and XOR, but this approach doesn't always work properly since rendering can happen in the window below the rubber band, but before the rubber band has been "erased".
You can create a QRubberBand whenever you need to render a rubber band around a given area (or to represent a single line), then call setGeometry(), move() or resize() to position and size it. A common pattern is to do this in conjunction with mouse events. For example:

  void Widget::mousePressEvent(QMouseEvent *event)
  {
      origin = event->pos();
      if (!rubberBand)
          rubberBand = new QRubberBand(QRubberBand::Rectangle, this);
      rubberBand->setGeometry(QRect(origin, QSize()));
      rubberBand->show();
  }

  void Widget::mouseMoveEvent(QMouseEvent *event)
  {
      rubberBand->setGeometry(QRect(origin, event->pos()).normalized());
  }

  void Widget::mouseReleaseEvent(QMouseEvent *event)
  {
      rubberBand->hide();
      // determine selection, for example using QRect::intersects()
      // and QRect::contains().
  }

## paint

1. 参考： QT **Coordinate System**的介绍

### Coordinate System

坐标系由QPainter类来控制。QPainter与QPaintDevice和QPaintEngine类一起构成了Qt绘画系统的基础， QPainter用于执行绘制操作，QPaintDevice是二维空间的抽象概念，可使用QPainter在其上绘制，而QPaintEngine则为绘制者在不同类型的设备上绘制提供接口。

QPaintDevice 类是可绘制对象的基类：它的绘图功能由 QWidget、QImage、QPixmap、QPicture 和 QOpenGLPaintDevice 类继承。**绘画设备的默认坐标系原点位于左上角。x 值向右增加，y 值向下增加。** 基于像素的设备默认单位为一个像素，打印机默认单位为一个点（1/72 英寸）。 逻辑 QPainter 坐标到物理 QPaintDevice 坐标的映射由 QPainter 的变换矩阵、视口和 "窗口 "处理。默认情况下，逻辑坐标系和物理坐标系是一致的。QPainter 还支持坐标变换（如旋转和缩放）。

### Transformations

默认情况下，QPainter在其相关联的设备的自身坐标系上进行操作，但它也完全支持仿射坐标变换。

QPainter有以下几个转换方法:

1. **rotate(angle)**: 顺时钟转动painter的坐标系angle度
2. **scale(dx, dy)**: 缩放painter的坐标系, 比如dx = src_width / dst_width, **scale前的宽度为src_width,scale后的绘制区域宽度为dst_width**
3. **translate(dx, dy)**: 平移坐标系原点，假设原来为(0,0), 则将原点移至(dx,dy)处

QPainter的变换矩阵的保存的恢复有以下两个方法:

1. **save()**
2. **restore()**

#### 示例

1. demo 1 -- The Analog Clock example

   时钟演示，经过translate和scale后，可以固定以200*200的正方形区域绘制一个时钟，时分针大小也可以200为背景参考设置大小，绘制完成后会自适应到窗口大小。

    ```c++
    void AnalogClockWindow::render(QPainter *p)
    {
        static const QPoint hourHand[3] = {
            QPoint(7, 8),
            QPoint(-7, 8),
            QPoint(0, -40)
        };
        static const QPoint minuteHand[3] = {
            QPoint(7, 8),
            QPoint(-7, 8),
            QPoint(0, -70)
        };

        QColor hourColor(127, 0, 127);
        QColor minuteColor(0, 127, 127, 191);

        p->setRenderHint(QPainter::Antialiasing);
        //1. 原点平移到窗口中心为绘制原点
        p->translate(width() / 2, height() / 2);

        //2. 缩放坐标系大小， side是原始窗体widthxheight转换成正方形的大小，
        //200是转换成的绘制目标区域的大小, 即后续的绘制区域为200X200
        int side = qMin(width(), height());
        p->scale(side / 200.0, side / 200.0);

        //3. 通过平移和缩放，我们将绘制坐标系转换成了一个
        // 区域大小为200x200，并以其正方形的中心为原点的坐标系

        p->setPen(Qt::NoPen);
        p->setBrush(hourColor);

        QTime time = QTime::currentTime();
        //绘制时针，以中心点为原点，每小时将坐标系顺时钟转动30度(360/12)，画一个三角形时针。
        //画完恢复原坐标系
        p->save();
        p->rotate(30.0 * ((time.hour() + time.minute() / 60.0)));
        p->drawConvexPolygon(hourHand, 3);
        p->restore();

        //画表盘时钟刻度, rotate了360度，故不用save/restore
        p->setPen(hourColor);
        for (int i = 0; i < 12; ++i) {
            p->drawLine(88, 0, 96, 0);
            p->rotate(30.0);
        }

        p->setPen(Qt::NoPen);
        p->setBrush(minuteColor);
        //绘制时针，以中心点为原点，每分钟转动6度(360/60)，画一个三角形时针。
        p->save();
        p->rotate(6.0 * (time.minute() + time.second() / 60.0));
        p->drawConvexPolygon(minuteHand, 3);
        p->restore();

        //画表盘分钟刻度
        p->setPen(minuteColor);
        for (int j = 0; j < 60; ++j) {
            if ((j % 5) != 0)
                p->drawLine(92, 0, 96, 0);
            p->rotate(6.0);
        }
    }

    ```

2. demo 2 -- 文字自适应窗口大小

    演示文本自适应窗体大小且居中显示，或者想以窗体的宽度的一半区域写就取width=this->widht()/2;

    ```c++
    void LightButton::drawText(QPainter *painter)
    {
        int width = this->width();
        int height = this->height();

        painter->save();
        painter.translate(width / 2, height / 2);
        QFont font;
        font.setPixelSize(85);
        painter->setFont(font);
        painter->setPen(textColor);
        auto fm = painter->fontMetrics();
        auto h = fm.height();//文字高度
        auto w = fm.horizontalAdvance(text); //文本以font为size的真实长度，
        //将坐标系转换为以font的size为比例的坐标系，如此就可定绘制区域rect了。
        painter->scale(width/w, height/h);
        //在font尺度下选择绘制区域
        QRect rect(-w/2, -h/2, w, h);
        painter->drawText(rect, Qt::AlignCenter, text);
        painter->restore();
    }
    ```

## dbus

### 前提条件 -- dbus总线

1. 安装dbus总线服务
    - linux下直接用命令行安装dbus服务，可以源码编译安装
    - window下源码编译安装
      - 安装cmake，编译环境
      - 下载dbus、expat
      - 编译安装
      - qt中，最好将lib库安装到编译器库下，比如“C:/Qt/Qt5.14.2/Tools/mingw730_64”, 这样在编译QT时demo时无需额外引入dbus库

2. 启动dbus服务
    - linux下使用`systemctl start dbus.service`等启动dbus-daemon
    - window下启动dbus-launch.exe服务即可。

### QT dbus

#### tools

tool | 说明
-|-
qdbusviewer.exe | D-Bus Viewer, 类似linux下的d-feet工具
qdbuscpp2xml.exe | 将*.h头文件转换成xml文件
qdbusxml2cpp.exe | 解析xml文件，生成静态代码(adaptor class或proxy class)

#### Using Qt D-Bus Adaptors

adaptor是附加到任何QObject派生类上的特殊类, 它是一个轻量的类，其主要目的是用来中继与真实对象的调用和被调用，并可验证或转换来自外部世界的输入，从而保护真实对象。

##### 使用步骤

1. 创建一个继承自**QDBusAbstractAdaptor**的类
1. 必须声明**Q_OBJECT**
1. 类中必须要有带"D-Bus Interface"声明的一个**Q_CLASSINFO**，用以声明接口类
1. 类中的**public slot**会被以MethodCall类型消传导到dbus上
1. 类中的所有**signal**信号都会自动中继到D-Bus上
1. 类中用**Q_PROPERTY**声明的属性也会自动暴露到D-Bus的属性接口上
1. 接口中的参数类型有特定要求，如下:
    Qt type | D-Bus equivalent type | 说明
    -|-|-
    uchar | BYTE
    qbool | BOOLEAN
    short | INT16
    ushort | UINT16
    int | INT32
    uint | UINT32
    qlonglong | INT64
    qulonglong | UINT64
    double | DOUBLE
    QString | STRING
    QDBusVariant | VARIANT
    QDBusObjectPath | OBJECT_PATH
    QDBusSignature | SIGNATURE
    QByteArray | array | 非原始类型
    QStringList | string array | 非原始类型

    参考[dbus.md](../vala/dbus.md)

#### 写Server端的便宜方法

1. 正常编写MyClass类, MyClass.h, MyClass.cpp, *注意,按先前的使用步骤中所述，将要导出的plot定义成public*

    ```h
    class MyClass : public GpioPin
    {
        Q_OBJECT
        Q_CLASSINFO("D-Bus Interface", "com.interface.name")
        Q_PROPERTY(int frequency READ frequency WRITE setFrequency NOTIFY frequencyChanged)
    public:
        explicit Pwm(int freq = 1, double dutycycle = 1.0, double value=1.0);
        ~Pwm();
        void setFrequency(int freq);
        int frequency(void) const { return m_frequency; }

    public Q_SLOTS:
        double getAverage(void) const;
        void start(void);
        void stop(void);

    Q_SIGNALS:
        void frequencyChanged(const int value);
    };
    ```

1. 使用`qdbuscpp2xml.exe`将MyClass.h转化成XXX.xml如下,也可手动更改

    ```sh
    qdbuscpp2xml.exe -A MyClass.h -o XXX.xml
    ```

    XXX.xml内容如下，XXX名称决定了后面转换的adaptor和proxy的前缀。

    ```xml
    <!DOCTYPE node PUBLIC "-//freedesktop//DTD D-BUS Object Introspection 1.0//EN" "http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd">
    <node>
    <interface name="com.interface.name">
        <property name="frequency" type="i" access="readwrite"/>
        <signal name="frequencyChanged">
        <arg name="value" type="i" direction="out"/>
        </signal>
        <method name="getAverage">
        <arg type="d" direction="out"/>
        </method>
        <method name="start">
        </method>
        <method name="stop">
        </method>
    </interface>
    </node>
    ```

1. qmake中引入XXX.xml文件, 在执行qmake时自动从xml中转化出XXX_adaptor类

    ```qmakefile
    QT += dbus

    DBUS_ADAPTORS += $$PWD/XXX.xml
    ```

1. 代码中引入adaptor类，并创建服务：

    ```cpp
    #include "XXX_adaptor.h"

    auto dev = MyClass();
    new  XXXAdaptor(dev);//在XXX_adaptor.h中定义，这个是qt工具自动用xml转换的。
    QDBusConnection connection = QDBusConnection::sessionBus();
    connection.registerObject("/path/to/object", dev);
    connection.registerService("org.example.servername");
    ```

#### 写接口端的便宜方法

1. 将XXX.xml从服务端复制过来

1. qmake中引入XXX.xml文件, 同理执行qmake时自动转化出XXX_interface类

   ```make
    QT += dbus

    DBUS_INTERFACES += XXX.xml
   ```

1. 代码中引入interface类使用

   ```cpp
   #include "XXX_interface.h"

   auto xxx = new XXXInterface("org.example.servername", "/path/to/object",  QDBusConnection::sessionBus(), this);
   xxx->start();
   ```

## enum与QString的互相转换

类定义如下：

```C++
#include <QObject>
class Cenum: public QObject
{
    Q_OBJECT
public:
    Cenum() {}

    enum Priority
    {
        High,
        Low,
        VeryHigh,
        VeryLow
    };
    Q_ENUM(Priority)
};
```

使用：

```c+
 qDebug()<<Cenum::High<<"\t"<<Cenum::Low;                     //！qDebug可以直接打印出枚举类值的字符串名称
 QMetaEnum metaEnum = QMetaEnum::fromType<Cenum::Priority>();
 qDebug()<<  metaEnum.valueToKey(Cenum::VeryHigh);            //! enum转string
 qDebug()<<  metaEnum.keysToValue("VeryHigh");                //！string转enum
```

## Qt Style Sheets

Topics:

- Overview
- **The Style Sheet Syntax**: 语法规则
- Qt Designer Integration
- Customizing Qt Widgets Using Style Sheets
- Qt Style Sheets Reference
  - List of Stylable Widgets
  - List of Properties
  - List of Icons
  - List of Property Types
  - List of Pseudo-States
  - List of Sub-Controls
- Qt Style Sheets Examples

### Selector

选择器 | 示例 | 说明
-|-|-
通用选择器 | * | 匹配所有控件。
类型选择器| QPushButton | 匹配 QPushButton 实例及其子类的实例。可支持自定义类，内建支持的参见 **List of Stylable Widgets**
属性选择器| QPushButton[flat="false"] | 匹配所有 flat 属性为 false 的 QPushButton 实例。您可以使用此选择器来测试任何支持 QVariant::toString() 的 Qt 属性（有关详细信息，请参阅 toString() 函数文档）。此外，还支持特殊的 class 属性，用于表示类名。此选择器还可用于测试动态属性。有关使用动态属性进行自定义的更多信息，请参阅使用动态属性进行自定义。<br> 除了 =，您还可以使用 ~= 来测试 Qt 属性类型 QStringList 是否包含给定的 QString。<br> 警告：如果 Qt 属性的值在设置样式表后发生变化，可能需要强制重新计算样式表。实现这一点的一种方法是取消设置样式表并重新设置。
类选择器 | .QPushButton | 匹配 QPushButton 实例，但不匹配其子类的实例。等价于 *[class~="QPushButton"].
ID 选择器| QPushButton#okButton | 匹配所有对象名称为 okButton 的 QPushButton 实例。
后代选择器| QDialog QPushButton | 匹配所有 QDialog 的后代（子代、孙代等）中的 QPushButton 实例。
子选择器| QDialog > QPushButton | 匹配所有直接子代为 QDialog 的 QPushButton 实例。

- ID选择器:  "**#**" 后面的**ID名字**必须与**object name**一致，可使用`setObjectName()`设置

    > ID Selector: QPushButton#okButton: Matches all QPushButton instances whose object name is okButton.

**注：不支持直接继承自QWidget的自定义类。**

### QWidget

参考**Qt Style Sheets Reference**中的**List of Stylable Widgets**, 直接继承自QWidget的控件不支持[selector types](qthelp://org.qt-project.qtwidgets.5142/qtwidgets/stylesheet-syntax.html#selector-types),即无法像其它控件那样指定选择它作为目标使用某样式

> Supports only the background, background-clip and background-origin properties.
If you subclass from QWidget, you need to provide a paintEvent for your custom QWidget as below:

eg:

- 样式设置-style.css:

  ```css
  #gerenalSettingPage {
      background: blue;
  }
  ```

- 在ui中使用：

  ```c++
    QFile file(":/style.css");
    file.open(QFile::ReadOnly);
    auto stylesheet = QString::fromUtf8(file.readAll());
    file.close();
    setStyleSheet(stylesheet);

    SettingPage *gerenalPage = new SettingPage(tr("通用设置"), this);
    gerenalPage->setObjectName("gerenalSettingPage");
  ```

- SettingPage实现：

    ```c++
    class SettingPage: public QWidget {
    Q_OBJECT
    public:
        SettingPage(const QString &title, QWidget *parent = nullptr) : QWidget(parent) {
            setWindowTitle(title);
            QVBoxLayout *mainLayout = new QVBoxLayout(this);
            mainLayout->setContentsMargins(8, 8, 8, 8); // 设置全局边距为0
            auto label = new QLabel(title, this);
            mainLayout->addWidget(label);
            setLayout(mainLayout);
        }
        ~SettingPage() {

        }
    };
    ```

    以上方法，css无作用，因为QWidget不支持选择器使用，改为QFrame即可。

    ```c++
    class SettingPage: public QFrame {
    Q_OBJECT
    public:
        SettingPage(const QString &title, QWidget *parent = nullptr) : QFrame (parent) {
            ...
        }
    };
    ```
