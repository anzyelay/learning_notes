# MERMAID

Mermaid: <https://mermaid.js.org/intro>

## mermaid example

## 甘特图-gantt

```mermaid
gantt
  title Adding GANTT diagram functionality to mermaid
  section 现有任务
  已完成               :done,    des1, 2014-01-06,2014-01-08
  进行中               :active,  des2, 2014-01-09, 4d
  计划中               :         des3, after des2, 5d
  section 未来任务
  计划中               :         des3, after des2, 5d
```

## 时序图-Sequence

example 1

```mermaid
sequenceDiagram
  actor a as 张三
  participant b as 李四
  a ->> b: 你好！李四, 最近怎么样?
  b -->> 王五: 你最近怎么样，王五？
  b --x a: 我很好，谢谢!
  b -x 王五: 我很好，谢谢!
  Note right of b: 李四想了很长时间, 文字太长了<br/>不适合放在一行.
  
  autonumber
  b-->>王五: 打量着王五...
  a->>王五: 很好... 王五, 你怎么样?
```

exmaple 2

```mermaid
sequenceDiagram
    actor s as server
    participant b as D-Bus 
    actor a as application1
    actor c as application2
    autonumber
    alt hasn`t register
        a->>b:query object
        b--)a: no object
        a -x a:  End
    else register a object
        s->>b: register object
        b--)b: record this object
        a->>b: subscribe signal1 
        c->>b: subscribe signal1
        c->>b: subscribe signal2
        loop has signal ?
            s->>b: emit signal1
            par time 1
                b--)a: notify signal1
            and time 1
                b--)c: notify signal1
            end
            s->>b: emit signal2
            b--)c: notify signal2
        end
        a->>b: end
        b-->>b: remove record
        b--)a: end
        c->>b: end
        b-->>b: remove record
        b--)c: end
    end

```

## 流程图-flowchart

```mermaid
flowchart LR
A[长方形] --- 链接无方向 --- B((圆))
A -.点链.-> C(圆角长方形)
B -...-> D{菱形}
subgraph 备注与箭头形状
  C <---> |双箭头|D
  C o--双箭头--o D
  C x--双箭头--x D
end
D ==一对多==> E[\bold/] & F[/F/] & J>text]
```

## 饼图-pie

```mermaid
pie
"Dogs" : 386
"Cats" : 85.9
"Rats" : 15
```

## 函数类图-[Class diagram](https://mermaid.js.org/syntax/classDiagram.html#syntax)

```mermaid
classDiagram
    %% %%: comment only usefull at top level
    %% visible:
    %%  -fun/mem: private
    %%  +fun/mem: public
    %%  #fun/mem: protect
    %% func()* : abstract
    %% func()$ : static
    note "From Duck till Zebra"
    Animal <|-- Duck
    note for Duck "can fly\ncan swim\ncan dive\ncan help in debugging"
    Animal <|-- Fish
    Animal <|-- Zebra : inheritance
    <<interface>> Animal
    Animal : +int age
    Animal : +String gender
    Animal: +isMammal() bool
    class Animal {
      +mate() string
      +run()*
    } 

    class Duck{
        <<description>>
        +String beakColor
        +swim() void
        +quack()
    }
    class Fish{
        -int sizeInFeet
        -canEat()
    }
    class Zebra{
        +bool is_wild
        +run()
    }

    Duck <|-- classB: Inheritance
    Duck *-- classD: Composition
    Duck o-- classF : Aggregation
    Duck <-- classH : Association
    Duck -- classJ : Link (Solid)
    Duck <.. classL : Dependency
    Duck <|.. classN : Realization
    Duck .. classP : Link (Dashed)
```

## mindmap

<!-- ```mermaid
mindmap
  root((mindmap))
    Origins
      Long history
      ::icon(fa fa-book)
      Popularisation
        British popular psychology author Tony Buzan
    Research
      On effectiveness<br/>and features
      On Automatic creation
        Uses
            Creative techniques
            Strategic planning
            Argument mapping
    Tools
      Pen and paper
      Mermaid
``` -->
