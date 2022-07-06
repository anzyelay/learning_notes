# 一、function define

## 1、parameter-direction(参数方向)
- in:
  - 引用类型(reference type) 、值类型(value type which is not a fundamental type)：一份引用传递,方法内修改对调用者可见，方法内不可分配
  - 基本类型(fundamental type): 一份拷贝传递，方法内修改对调用者不可见
- out:
- ref:in and also out

## 2、[Contract programming(契约式编程)](https://wiki.gnome.org/Projects/Vala/Manual/Methods#Contract_programming)
契约是减少大型项目成本的突破性技术。它一般由 Precondition(前置条件)， Postcondition(后置条件) 和 Invariant(不变量) 等概念组成.
vala中
> A method may have preconditions (requires) and postconditions (ensures) that must be fulfilled at the beginning or the end of a method respectively

- **requires**: 前置条件
- **ensures**：后置条件
- **result** :is a special variable representing the return value. 
```vala
double method_name (int x, double d)
                requires (x > 0 && x < 10)
                requires (d >= 0.0 && d <= 1.0)
                ensures (result >= 0.0 && result <= 10.0) {
        return d * x;
}
```
## 3、变参函数
```vala
int sum (int x, ...) {
    int result = x;
    va_list list = va_list ();
    for (int? y = list.arg<int?> (); y != null; y = list.arg<int?> ()) {
        result += y;
    }
    return result;
}

int a = sum (1, 2, 3, 36);
```

# 二、function type
## 1. enum_to_string函数的定义
```vala
    private enum AccentColor {
        NO_PREFERENCE,
        RED,
        GRAY;

        public string to_string () {
            switch (this) {
                case RED:
                    return "strawberry";
                case GRAY:
                    return "slate";
            }

            return "auto";
        }
    }
```
## 2. [async函数](https://wiki.gnome.org/Projects/Vala/Tutorial#Asynchronous_Methods)
有返回值的定义与调用如下，无的话直接调用begin就行。
```vala
async typeRet funcA(typeArg args...){
    ...
}   
//call as bellow:
    funcA.begin(typeArg args... , (obj, res) => {
        typeRet ret = funcA.end(res);
        //judge ret 
            ...
        

    })；
```
### yield意义：
- 使用范围： 只能在async函数中使用
- 用法：
    1. `yield async_func`    : 与异步调用一起使用，它将暂停当前协程，直到调用完成并返回结果。
    2. `yield`               : 单独使用， 暂停当前协程，直到通过调用其源回调将其唤醒。
    ```vala
    // Build with: valac --pkg=gio-2.0 example.vala

    public async void nap (uint interval, int priority = GLib.Priority.DEFAULT) {
    GLib.Timeout.add (interval, () => {
        nap.callback (); //唤醒下面的yield
        return false;
        }, priority);
    yield; //暂停当前协程，等待唤醒
    }

    private async void do_stuff () {
    yield nap (1000); // 暂停直到返回结果为止
    }

    private static int main (string[] args) {
    GLib.MainLoop loop = new GLib.MainLoop ();
    do_stuff.begin ((obj, async_res) => {
        loop.quit ();
        });
    loop.run ();

    return 0;
    }
    ```

## 3. how to call override class parent function
```vala    
public override void func () {
    base.func();
    /*
        you func code here 
    */
}
```


# 参考：
1. [Vala Tutorial](https://wiki.gnome.org/Projects/Vala/Tutorial)
