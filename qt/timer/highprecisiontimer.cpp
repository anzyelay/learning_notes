#include "highprecisiontimer.h"
#include <QApplication>
#include <QDebug>


#ifdef Q_OS_WIN
#include <windows.h>
qint64 HighPrecisionTimer::abtimeUs() {
    static LARGE_INTEGER freq = {{0, 0}};
    LARGE_INTEGER clock;
    if (freq.QuadPart == 0) {
        QueryPerformanceFrequency(&freq);
    }
    QueryPerformanceCounter(&clock);
    return (qint64)clock.QuadPart*1000000/freq.QuadPart;
}
#elif defined(Q_OS_LINUX)
#include <time.h>
#include <sys/time.h>
qint64 HighPrecisionTimer::abtimeUs() {
    double result = 0.0;
    struct timeval tv;
    gettimeofday( &tv, NULL );
    result = tv.tv_sec*1000*1000 + tv.tv_usec;
    return result;
}
#endif /* _WIN32 */

int HighPrecisionTimer::usSleep(int us)
{
    LARGE_INTEGER fre;
    if (QueryPerformanceFrequency(&fre) ) {
        LARGE_INTEGER run,priv,curr;
        run.QuadPart = fre.QuadPart * us / 1000000;
        QueryPerformanceCounter (&priv);
        do {
            QueryPerformanceCounter(&curr);
        } while (curr.QuadPart - priv.QuadPart < run.QuadPart) ;
        curr.QuadPart -= priv.QuadPart;
        int nres = (curr.QuadPart * 10000000 / fre.QuadPart) ;
        return nres;
    }
    return -1;
}


HighPrecisionTimer::HighPrecisionTimer(QObject *parent) : QObject(parent)
{
    timerId = 0;
    if (parent) {
        connect(parent,&QObject::destroyed, [=](){
            stop();
        });
    }
}

HighPrecisionTimer::~HighPrecisionTimer()
{
    stop();
}

void HighPrecisionTimer::start(qint64 microseconds)
{
    this->startTime = abtimeUs();
    this->interval = microseconds;
    timerId = HighPreTimerWorker::instance().addTimer(this);
}

void HighPrecisionTimer::startOnce(qint64 microseconds, std::function<void ()> task)
{
    QtConcurrent::run([=](){
        usSleep(microseconds);
        task();
    });
}

void HighPrecisionTimer::start(qint64 microseconds, std::function<bool ()> task)
{
    QtConcurrent::run([=](){
        do {
            usSleep(microseconds);
        } while (task());
    });
}

void HighPrecisionTimer::stop() {
    if (timerId <= 0)
        return;
    HighPreTimerWorker::instance().delTimer(this);
    timerId = 0;
}

void HighPrecisionTimer::checkHighPrecisionTimeout(const qint64 &currentTime) {
    qint64 elapsedTime = (currentTime - startTime);
    if (elapsedTime >= interval) {
        // 定时器到期，执行相应的操作
        emit timeout();
        // 重置定时器或停止它...
        startTime = currentTime;
    }
}

HighPreTimerWorker &HighPreTimerWorker::instance(QObject *parent) {
    static HighPreTimerWorker once(parent);
    return once;
}

HighPreTimerWorker::~HighPreTimerWorker()
{
    foreach (auto timer, m_timers) {
        timer->stop();
    }
    wait();
    qDebug() << "thread destruction: " << currentThreadId();
}

int HighPreTimerWorker::addTimer(HighPrecisionTimer *timer)
{
    timer->moveToThread(this);
    m_timers.append(timer);
    if (m_timers.size() == 1) {
        start(TimeCriticalPriority);
    }
    return m_timers.size();
}

void HighPreTimerWorker::delTimer(HighPrecisionTimer *timer)
{
    m_deleteCache.enqueue(timer);
}

void HighPreTimerWorker::run()
{
    while (isRunning()) {
        qint64 currentTime = HighPrecisionTimer::abtimeUs();
        foreach (auto timer, m_timers) {
            timer->checkHighPrecisionTimeout(currentTime);
        }
        yieldCurrentThread();
        while (!m_deleteCache.isEmpty()) {
            m_timers.removeOne(m_deleteCache.dequeue());
        }
        if (m_timers.empty()) {
            break;
        }
    }
    qDebug() << "quit from thread: " << currentThreadId();
}

HighPreTimerWorker::HighPreTimerWorker(QObject *parent) : QThread(parent)
{
    m_timers.clear();
    connect(QApplication::instance(), &QApplication::destroyed, this, &QThread::quit);
}
