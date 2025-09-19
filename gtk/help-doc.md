# glib help doc

## 对象管理

### 到底谁是 instance？

> “instance” 指的是调用该方法的对象实例，也就是你调用方法时的“接收者”
>```c
>JsonNode *node = ...;
>JsonObject *obj = json_node_get_object(node);
>```
>此时：
>- node 是你调用方法的对象 → 它就是 instance
>- 返回的 obj 是 node 内部持有的对象
>
> ```c
> JsonNode *node = ...;
> JsonObject *obj = json_object_new();
> json_node_set_object(node, obj);
> ```
> 此时：
> -  node 是 instance（你调用方法的对象）
> - obj 是你传入的参数

在查看文档说明时有两种说法如下：

- The instance takes ownership of the data, and is responsible for freeing it
  - 参数的所有权转移到了调用方法的对象实例，由它负责free参数
  - 调用者不再需要释放它

- The returned data is owned by the instance.
  - 返回值的所有权仍由调用对象实例持有。
  - 接收者不需要释放它

- The data is owned by the caller of the method
  - 参数的所有权未转换，调用者仍拥有传入参数的所有权
    - 你负责管理它的生命周期
    - 需要在合适的时机调用 json_object_unref() 来释放它
    - 只是对它增加了引用计数，并不会替你释放它。
