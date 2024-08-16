# kbuild学习记录

## 规则说明

|规则名|解析名|说明
|-|-|-|
|target: 1rule<br>target: 2rule<br>...<br>target: nrule|多规则目标|一个文件可以作为多条规则的目标【即相同的target，不同的prerequsites】，
|:|普通规则| 1. 当前置依赖有变化更新（update）时就会执行后面的command<br> 2. 普通规则的多规则目标最终会将所有依赖文件将合并成一个依赖文件列表构成一个该目标的规则【即最终target: all merged prerequsites】，最终也只能有最后一条规则命令有效，并且会发出警告信息<br>3. 对于一个没有赖条件的普通规则，当规则的目标文件存在时，此规则的命令永远不会被执行（目标文 件永远是最新的）。
|::|双冒号规则| 1. 普通规则和双冒号规则不可混用，多规则目标只能选择其中一种；<br>2. 双冒号规则允许在多条规则中为同一个目标指定不同的重建目标的命令,而不是合并成最后一条，哪条规则的依赖条件更新了就执行哪条规则  <br>3. 在双冒号多规则目标中， 当依赖条件比目标更新（newer）时，就会执行规则命令, 因此， 对于一个没有依赖 而只有命令行的双冒号规则，当引用此目标时，规则的命令将会被无条件执行
|Rules without Commands or Prerequisites|空条件规则| 是一个强制目标，这个规则一旦被执行， make就认为<b>它的目标(target)</b>已经被更新过。这样的目标在作为一个规则的依赖时，因为依赖总被认为被更新过，因此作为依赖所在的规则中定义的命令总会被执行。
|.PHONY|伪目标| 1. 它不代表一个真正的目标<br>2. 在执行 make 时可以指定这个目标来执行其所在规则定义的命令<br>3. 当一个目标被声明为伪目标后， make 在执行此规则时不 会试图去查找隐含规则来创建它<br>4. 一般情况下，一个伪目标不作为另外一个目标的依赖。然而当伪目标作为某个目标 的依赖时，其命令体都会被强制执行,
|TARGETS ... : TARGET-PARTTEN: PREREQ-PATTERNS ...|静态模式|1. TAGETS列出了此规则的一系列目标文件。像普通规则的目标一样可以包含通配 符。<br>2. TAGET-PATTERN 和 PREREQ-PATTERNS 说明了如何为每一个目标文件生成依赖 文件。

1. 伪目标,空规则和多规则目标作用示例

```makefile
# 强制目标
 $(obj)/%.o: $(src)/%.c FORCE
    $(call cmd,force_checksrc)
    $(call if_changed_rule,cc_o_c)
 # Add FORCE to the prequisites of a target to force it to bealways rebuilt.
 PHONY += FORCE

 # 空条件规则
 FORCE:

 # 伪目标
 .PHONY: $(PHONY)

 # all为多规则目标，make all最终会生成3个target:vmlinux bzImage modules
 all: vmlinux

 all: modules

 all: bzImage

 # 静态模式
 $(single-used-m): $(obj)/%.o: $(src)/%.c FORCE
    $(call cmd,force_checksrc)
    $(call if_changed_rule,cc_o_c)
    @{ echo $(@:.o=.ko); echo $@; } > $(MODVERDIR)/$(@F:.o =.mod)

 # 以driver/net/Makefile 中的几个编译成 module 的文件为例： 
 # sing-used-m := 3c509.o e100.o 
 # 此时静态规则会为 3c508.o e100.o 分别产生对应的规则
 # $(obj)/3c509.o : $(src)/3c509.c FORCE 
 #    ...此处后面的command省略
 # $(obj)/e100.o : $(src)/e100.c FORCE
 #    ...此处后面的command省略

```

## 参考

[kbuild实现分析.pdf](./kbuild实现分析.pdf)
