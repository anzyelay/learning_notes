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
## 2. async函数
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
