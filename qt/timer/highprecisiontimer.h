#ifndef HIGHPRECISIONTIMER_H
#define HIGHPRECISIONTIMER_H

#include <QThread>
#include <QtConcurrent/QtConcurrent>
// 由于QT的精度在15ms以上（WINDOW平台特设默认精度为15ms，故QT只能到此级别），
// 要做到1ms以下，只能自己实现。
// 用于C++11标准库的时间点计算（示例中未直接使用，但可以是备选方案）
// chrono的精度也无法做到US级别
// #include <chrono> 

// 精度测试到15us ，仅限只有一到两个定时器时，定时器一多，QT的信号传递耗时也会有影响
class HighPrecisionTimer : public QObject
{
    Q_OBJECT
public:
    explicit HighPrecisionTimer(QObject *parent = nullptr);
    ~HighPrecisionTimer();

    void start(qint64 microseconds);
    void stop();

    static qint64 abtimeUs();
    static int usSleep(int us);
    static void startOnce(qint64 microseconds, std::function<void ()> task);
    // start to creat a separate thread to run task in microseconds repeatly until the task return false.
    static void start(qint64 microseconds, std::function<bool ()> task);

    void checkHighPrecisionTimeout(const qint64 &currentTime);

    int id() { return timerId; }

private:
    int timerId;
    qint64 startTime;
    qint64 interval;

signals:
    void timeout(); // 当高精度定时器到期时发出信号
};

class HighPreTimerWorker : public QThread
{
public:
    static HighPreTimerWorker &instance(QObject *parent = nullptr);
    ~HighPreTimerWorker();
    int addTimer(HighPrecisionTimer *timer);
    void delTimer(HighPrecisionTimer *timer);
protected:
    void run() override;
private:
    explicit HighPreTimerWorker(QObject *parent = nullptr);
    QList<HighPrecisionTimer *> m_timers;
    QQueue<HighPrecisionTimer *> m_deleteCache;
};

#endif // HIGHPRECISIONTIMER_H
