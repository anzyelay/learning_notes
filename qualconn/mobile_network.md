# MOBILE NETWORK

[参考链接](https://blog.csdn.net/weixin_41629848/article/details/108882459)

## 名词解释

### UE

> User equipment that are wireless connected. e.g. mobile phones, laptop. 

### MCC (Mobile Country Code)

> 移动国家代码, 用于识别归属国家
> 比如460为中国，440为日本

### MNC (Mobile Network Code)

> 移动网络代码, 用于识别归属网络。
> 比如:
>
>- 移动:00、02、04、07
>- 联通:01、06、09
>- 电信:03、05、11
>

### IMEI (International Mobile Equipment Identity)

> 国际移动设备识别码,  15位数字组成，比如：860648047240754

- **TAC**, 6位，类型分配码(Type Allocation Code)， 比如86为中国
- **FAC**, 2位，装配厂家号码
- **SNR**, 6位， 产品序号
- **SP**, 1位，检验码
- 全球唯一，绑定于设备

### IMSI (International Mobile Subscriber Identification Number)

> 国际移动用户识别号, 由15位数字组成， 比如：440209199154307

- 第一部分，3位数，**MCC**
- 第二部分，2位数，**MNC**
- 第三部分，10位数 **MSIN**, 移动订阅用户识别代码(Mobile subscription identification number).
- 全球唯一，绑定于SIM卡

### PLMN (Public Land Mobile Network)

> - 公共陆地移动通信网络
> - 由政府或它所批准的经营者，为公众提供陆地移动通信业务目的而建立和经营的网络。该网路通常与公众交换电话网（PSTN）互连，形成整个地区或国家规模的通信网
> - 是移动通信网络的代名词。具体到我们国家，每个移动通信运营商的网络算一个PLMN，因此中国移动、中国联通和中国电信的网络是不同的PLMN
> - 随着虚拟运营商的兴起，虚拟运营商也可以算成是一个PLMN。
> - 为了让用户搞清楚所处的PLMN，每个PLMN都应该有明确的标识（编号），并作为系统信息由基站来广播，让基站下的终端都能够接收到。

- **PLMN = MCC + MNC**
- 对于一个特定的终端（UE/手机），通常需要维护/保存几种不同类型的PLMN表，每个列表中会有多个PLMN。如下：

    简称 | 全称 | 说明
    -|-|-
    RPLMN |(Registered PLMN) 已登记PLMN | 终端在上次关机或脱网前登记上的PLMN
    EPLMN | (Equivalent PLMN) 等效PLMN |与终端当前所选择的PLMN处于同等地位的PLMN，其优先级相同； 这个PLMN在MSC 或者MME上配置，如果用户在归属地那么EPLMN=EHPLMN。如果在漫游地，EPLMN！=EHPLMN
    EHPLMN | (Equivalent Home PLMN) 等效本地PLMN | 为与终端当前所选择的PLMN处于同等地位的本地PLMN； HPLMN对应的运营商可能会有不同的号段，例如中国移动有46000、46002、46007 三个号段。  46002相对46000就是EHPLMN;运营商烧卡时写入USIM卡中
    HPLMN | (Home PLMN ) 归属PLMN | 为终端用户归属的PLMN； 终端USIM卡上的IMSI号中包含的MCC和MNC与HPLMN上的MCC和MNC是一致的
    VPLMN | (Visited PLMN) 访问PLMN |为终端用户访问的PLMN。其PLMN和存在SIM卡中的IMSI的MCC，MNC是不完全相同的
    UPLMN | (User Controlled PLMN) 用户控制PLMN|储存在USIM卡上的一个与PLMN选择有关的参数
    OPLMN | (Operator Controlled PLMN) 运营商控制PLMN|储存在USIM 卡上的一个与PLMN选择有关的参数
    FPLMN | (Forbidden PLMN) 禁用PLMN|为被禁止访问的PLMN，通常终端在尝试接入某个PLMN被拒绝以后，会将其加到本列表中
    APLMN | (Approve PLMN) 可捕获PLMN|为终端能在其上找到至少一个小区，并能读出其PLMN标识信息的PLMN。

### PSTN (Public Switched Telephone Network)

> 公共交换电话网络，一种常用旧式电话系统。即我们日常生活中常用的电话网, （个人理解即固定电话网络）

### RAT (Radio Access Technology)

> 无线电接入技术, 适用于无线通信网络的基础物理连接方法。
> 现代手机可以在一个设备中支持多种无线接入技术（RAT）
> 例如蓝牙、Wi-Fi、 NFC（近场通信）以及 3G、4G 或 LTE 和 5G。

## 注网

- **UE在开机时，首要任务是搜索网络并注册，即选网操作。UE的选网操作可以分为PLMN选择和小区搜索两个过程。在PLMN选择过程中，UE会维护一些PLMN列表，这些列表将PLMN按照优先级排序，然后从高优先级向下搜索，优先顺序为：RPLMN，HPLMN，UPLMN，OPLMN，VPLMN。除VPLMN外，每一类PLMN列表中都存储有对应的RAT（无线接入技术），即标识GSM/GPRS技术、UMTS技术还是E-UTRA技术，以实现同一PLMN下哪种接入技术优先。**

- 用户在接入网络时不仅要考虑接入本运营商网络还要考虑接入的时间。

- **RPLMN作为上一次注册过的网络，从接入成功率以及接入时间来说肯定是最优的，EPLMN是上次注册网络的对等网络等同于RPLMN。**

- 其他PLMN是当前终端能搜索到的所有PLMN，当前面各种PLMN都匹配不上的话，终端将按信号强弱进行尝试接入。

- 由于综合各种原因接入的网络不一定是用户的归属运营商网络，因此用户接入网络之后还会发起小区重选流程，这时如果有HPLMN,EHPLMN的信号覆盖的话，会选择回到归属运营商网络。

- 手机和基站都要设置PLMN，**手机端需要知道，我能接入的运营商的PLMN有哪些，而基站那边需要知道，我允许哪些运营商的手机漫游进来**
