QT       += core

HEADERS += \
    $$PWD/highprecisiontimer.h 

SOURCES += \
    $$PWD/highprecisiontimer.cpp 

##  说明:
##  由于QT的精度在15ms以上（WINDOW平台特性默认精度为15ms，故QT只能到此级别），
##  要做到1ms以下，只能自己实现。
##  用于C++11标准库的时间点计算（示例中未直接使用，但可以是备选方案）
##  chrono的精度也无法做到US级别
##  #include <chrono> 

##  本例程精度测试到15us ，仅限只有一到两个定时器时，定时器一多，QT的信号传递耗时也会有影响
## for example:
##   for (auto i : {15, 150}) {
##       auto t = new HighPrecisionTimer();
##       connect(t, &HighPrecisionTimer::timeout, [=](){
##           qDebug() << t->id() << ":" << i << "==" << HighPrecisionTimer::abtimeUs();
##       });
##       t->start(i);
##   }
