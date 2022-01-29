# [GObject](https://docs.gtk.org/gobject/concepts.html)
## The GLib Dynamic Type System

在程序的类型系统中注册新的 **GType** 需要使用以下三个C函数中的一个 ,它们在gtype.h中定义并在gtype.c中实现：
 - `g_type_register_static()` 
 - `g_type_register_dynamic()`  
 - `g_type_register_fundamental()` : 用于创建基础类型的类（如int, long等），基本用不上
```c
typedef struct _GTypeInfo               GTypeInfo;
struct _GTypeInfo
{
  /* interface types, classed types, instantiated types */
  guint16                class_size; //类大小

  GBaseInitFunc          base_init; //C++ constructor
  GBaseFinalizeFunc      base_finalize; //C++ destructor

  /* classed types, instantiated types */
  GClassInitFunc         class_init; //C++ constructor
  GClassFinalizeFunc     class_finalize; //C++ destructor
  gconstpointer          class_data;

  /* instantiated types */
  guint16                instance_size; // C++ parameter to new, 实例的大小
  guint16                n_preallocs; // C++ type of new operator,实例化策略
  GInstanceInitFunc      instance_init; // 初始化实例方法

  /* value handling */
  const GTypeValueTable *value_table;//  C++ copy， 使用g_value_copy(src, dst)时调用此方法,所以所有类都可通过GValue来copy数据
};

GType
g_type_register_static (GType            parent_type,
                        const gchar     *type_name,
                        const GTypeInfo *info,
                        GTypeFlags       flags);

GType
g_type_register_fundamental (GType                       type_id,
                             const gchar                *type_name,
                             const GTypeInfo            *info,
                             const GTypeFundamentalInfo *finfo,
                             GTypeFlags                  flags);

```
## 针对对象的初始化和销毁的调用时机
调用时机 |	被调用函数 |	函数的参数
-|-|-
首次针对目标类型调用`g_type_create_instance()`函数时 |	type’s `base_init` function |	On the inheritance tree of classes from fundamental type to target type. `base_init` is invoked once for each class structure.|
| |target type’s `class_init` function| 	目标类型的类结构(On target type’s class structure)
| |interface initialization, see the section called “Interface Initialization” |
|对目标类每次调用`g_type_create_instance()`时|	target type’s `instance_init` function 	|对象的实例(On object’s instance)
|对目标类最后一次调用`g_type_free_instance()`时 |	interface destruction, see the section called “Interface Destruction” 	|
||target type’s `class_finalize` function |	目标类型的类结构
||type’s `base_finalize` function |	On the inheritance tree of classes from fundamental type to target type. `base_finalize` is invoked once for each class structure.
## 针对接口的初始化和销毁的调用时机
 todo
## 在头文件中创建一个导出新类型的约定
- 函数名命名规则：`prefix_object_method` ， **prefix**为库名/工程名，类似namespace; **object**为对象名; **method**为方法名，(eg:`viewr_file_save`)
- 创建一个命名为`PREFIX_TYPE_OBJECT` 宏，该宏总是返回与其相联类型的GType,宏的实现方法名为`prefix_object_get_type`, (eg: `#define VIEWER_TYPE_FILE viewr_file_get_type()`)
-  使用 `G_DECLARE_FINAL_TYPE` 或 `G_DECLARE_DERIVABLE_TYPE` 为您的对象定义各种其他约定宏，有下面这些：
    - `PREFIX_OBJECT (obj)`： 返回`PrefixObject`类型的对象指针, 用于静态或动态地安全地强制转化目标类型，在编译GLib库时可以禁用动态类型检查
    - `PREFIX_OBJECT_CLASS (klass)`： 与上面相似，返回`PrefixObjecClass`类的类结构指针
    - `PREFIX_IS_OBJECT (obj)`: 返回布尔类型，用于判断obj是否为非空的目标对象
    - `PREFIX_IS_OBJECT_CLASS (klass)`: 同上，是否为目标类指针
    - `PREFIX_OBJECT_GET_CLASS (obj)`: 它返回与给定obj关联的类指针
```c

#define VIEWER_TYPE_FILE viewer_file_get_type()
G_DECLARE_FINAL_TYPE (ViewerFile, viewer_file, VIEWER, FILE, GObject)


#if 1
//使用下面的宏定义一个新class
G_DEFINE_TYPE (ViewerFile, viewer_file, G_TYPE_OBJECT)
#else
//如果有特殊化的需求，也可以手动实现如下函数来新建一个class
GType viewer_file_get_type (void)
{
  static GType type = 0;
  if (type == 0) {
    const GTypeInfo info = {
      /* You fill this structure. */
    };
    type = g_type_register_static (G_TYPE_OBJECT,
                                   "ViewerFile",
                                   &info, 0);
  }
  return type;
}
#endif
```
## 注册示例：
### 1. 不可实例化，无类结构的基本类型数据的注册--`gchar`
```c
GTypeInfo info = {
  .class_size = 0,

  .base_init = NULL,
  .base_finalize = NULL,

  .class_init = NULL,
  .class_finalize = NULL,
  .class_data = NULL,

  .instance_size = 0,
  .n_preallocs = 0,
  .instance_init = NULL,

  .value_table = NULL,
};

static const GTypeValueTable value_table = {
  .value_init = value_init_long0,
  .value_free = NULL,
  .value_copy = value_copy_long0,
  .value_peek_pointer = NULL,

  .collect_format = "i",
  .collect_value = value_collect_int,
  .lcopy_format = "p",
  .lcopy_value = value_lcopy_char,
};

info.value_table = &value_table;

type = g_type_register_fundamental (G_TYPE_CHAR, "gchar", &info, &finfo, 0);

```
### 2. 注册可实例化的有类型结构类型 : 对象
以下是一个继承自G_TYPE_OBJECT的ViewrFile对象的注册示例
```c
typedef struct {
  GObject parent_instance;

  /* instance members */
  char *filename;
} ViewerFile;

typedef struct {
  GObjectClass parent_class;

  /* class members */

  /* the first is public, pure and virtual */
  void (*open)  (ViewerFile  *self,
                 GError     **error);

  /* the second is public and virtual */
  void (*close) (ViewerFile  *self,
                 GError     **error);
} ViewerFileClass;

#define VIEWER_TYPE_FILE (viewer_file_get_type ())

GType
viewer_file_get_type (void)
{
  static GType type = 0;
  if (type == 0) {
    const GTypeInfo info = {
      .class_size = sizeof (ViewerFileClass),
      .base_init = NULL,
      .base_finalize = NULL,
      .class_init = (GClassInitFunc) viewer_file_class_init,
      .class_finalize = NULL,
      .class_data = NULL,
      .instance_size = sizeof (ViewerFile),
      .n_preallocs = 0,
      .instance_init = (GInstanceInitFunc) viewer_file_init,
    };
    type = g_type_register_static (G_TYPE_OBJECT,
                                   "ViewerFile",
                                   &info, 0);
  }
  return type;
}

```
### 3. 注册不能实例化的有类结构的类型：接口
`viewer_editable_get_type`注册了一个名为`ViewerEditable`的接口类型，它继承自 G_TYPE_INTERFACE。所有接口都必须是继承树中G_TYPE_INTERFACE的子级。
```c
#define VIEWER_TYPE_EDITABLE viewer_editable_get_type ()
G_DECLARE_INTERFACE (ViewerEditable, viewer_editable, VIEWER, EDITABLE, GObject)

struct _ViewerEditableInterface {
  GTypeInterface parent;

  void (*save) (ViewerEditable  *self,
                GError         **error);
};

void viewer_editable_save (ViewerEditable  *self,
                           GError         **error);
```
`viewer_editable_save`的实现如下：
```c
void
viewer_editable_save (ViewerEditable  *self,
                      GError         **error)
{
  ViewerEditableinterface *iface;

  g_return_if_fail (VIEWER_IS_EDITABLE (self));
  g_return_if_fail (error == NULL || *error == NULL);

  iface = VIEWER_EDITABLE_GET_IFACE (self);
  g_return_if_fail (iface->save != NULL);
  iface->save (self);
}

```
## 对象的实例化方法
`g_object_new`:
 - 通过 `g_type_create_instance()` 分配和清除内存
 - 使用构造属性初始化对象的实例
