# readme

## 说明

由于QT的精度在15ms以上（WINDOW平台特性默认精度为15ms，故QT只能到此级别），
要做到1ms以下，只能自己实现。
用于C++11标准库的时间点计算（示例中未直接使用，但可以是备选方案）
chrono的精度也无法做到US级别
在WINDOW平台没找到类似linux下的yield机制，所以开个线程去做，但仍会占用cpu,
经测试目前我知道的任何延时函数都无法达到us级别，如有请告知。

本例程精度测试到15us ，仅限只有一到两个定时器时，定时器一多，QT的信号传递耗时也会有影响
for example:

1. use in QT qmake files:

    ```makefile
    include(timer.pri)
    ```

1. cpp

    ```cpp
    #include "highprecisiontimer.h"

    int index=0;
    for (auto i : {1, 10, 20, 2}) {
        auto t = new HighPrecisionTimer();
        auto old = HighPrecisionTimer::abtimeUs();
        auto c = connect(t, &HighPrecisionTimer::timeout, [=](){
            static int j[4] = {0};
            j[index]++;
            if (j[index]>=100000) {
                auto nt = HighPrecisionTimer::abtimeUs();
                qDebug () << t->id() << ":" << i*j[index] << "=="  << nt - old << old << nt;
                t->stop();
            }
        });
        index++;
        t->start(i);
    }

    auto old = HighPrecisionTimer::abtimeUs();
    int interval = 1;
    HighPrecisionTimer::start(interval, [=](){
        static int i = 0;
        if (i>=100000) {
            auto nt = HighPrecisionTimer::abtimeUs();
            qDebug () << i*interval << nt - old ;
            return false;
        }
        i++;
        return true;
    });
    ```

## chrono implemetion

```cpp
#ifndef WINPRECISIONTIMER_H
#define WINPRECISIONTIMER_H
#pragma once

#include <functional>
#include <chrono>
#include <thread>
#include <atomic>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <Windows.h>
using namespace std;

class TimeRecord
{
public:
    TimeRecord() : m_beginTime(chrono::high_resolution_clock::now()) {}
    void reset() { m_beginTime = chrono::high_resolution_clock::now(); }

    //return milliseconds
    int64_t elapsed() const
    {
        return chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - m_beginTime).count();
    }
    //return microseconds
    int64_t elapsed_micro() const
    {
        return chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now() - m_beginTime).count();
    }
    //return nanoseconds
    int64_t elapsed_nano() const
    {
        return chrono::duration_cast<chrono::nanoseconds>(chrono::high_resolution_clock::now() - m_beginTime).count();
    }
    //return seconds
    int64_t elapsed_seconds() const
    {
        return chrono::duration_cast<chrono::seconds>(chrono::high_resolution_clock::now() - m_beginTime).count();
    }
    //return minutes
    int64_t elapsed_minutes() const
    {
        return chrono::duration_cast<chrono::minutes>(chrono::high_resolution_clock::now() - m_beginTime).count();
    }
    //return hours
    int64_t elapsed_hours() const
    {
        return chrono::duration_cast<chrono::hours>(chrono::high_resolution_clock::now() - m_beginTime).count();
    }
private:
    chrono::time_point<chrono::high_resolution_clock> m_beginTime;
};


class Timer
{
public:
    Timer();

    Timer(const Timer& timer);

    ~Timer();


    //sleep for milliseconds
    void millSleep(long milliseconds);
    void usSleep(long microseconds);

    void startOnce(int delay, function<void()> task);

    void start(int interval, function<void()> task);

    void stop();

private:
    atomic<bool> m_expired; // timer stopped status
    atomic<bool> m_tryToExpire; // timer is in stop process
    mutex m_mutex;
    condition_variable m_expiredCond;
};

#endif // PRECISETIMER_H

```

```cpp
#include "winprecisiontimer.h"


Timer::Timer() : m_expired(true), m_tryToExpire(false)
{

}

Timer::Timer(const Timer &timer)
{
    m_expired = timer.m_expired.load();
    m_tryToExpire = timer.m_tryToExpire.load();
}

Timer::~Timer()
{
    stop();
}

void Timer::usSleep(long microseconds)
{
    LARGE_INTEGER litmp;
    LONGLONG QPart1, QPart2;
    double dfMinus, dfFreq, dfTim, dfSpec;
    QueryPerformanceFrequency(&litmp);
    dfFreq = (double)litmp.QuadPart;
    QueryPerformanceCounter(&litmp);
    QPart1 = litmp.QuadPart;
    dfSpec = microseconds/1000000;

    do
    {
        QueryPerformanceCounter(&litmp);
        QPart2 = litmp.QuadPart;
        dfMinus = (double)(QPart2 - QPart1);
        dfTim = dfMinus / dfFreq;
        this_thread::yield();
    } while (dfTim < dfSpec);
}

void Timer::millSleep(long milliseconds)
{
    LARGE_INTEGER litmp;
    LONGLONG QPart1, QPart2;
    double dfMinus, dfFreq, dfTim, dfSpec;
    QueryPerformanceFrequency(&litmp);
    dfFreq = (double)litmp.QuadPart;
    QueryPerformanceCounter(&litmp);
    QPart1 = litmp.QuadPart;
    dfSpec = 0.001*milliseconds;

    do
    {
        QueryPerformanceCounter(&litmp);
        QPart2 = litmp.QuadPart;
        dfMinus = (double)(QPart2 - QPart1);
        dfTim = dfMinus / dfFreq;
    } while (dfTim < dfSpec);
}

void Timer::startOnce(int delay, function<void ()> task)
{
    thread([delay, task]() {
        this_thread::sleep_for(chrono::milliseconds(delay));
        task();
    }).detach();
}

void Timer::start(int interval, function<void ()> task)
{
    // is started, do not start again
    if (m_expired == false)
        return;

    // start async timer, launch thread and wait in that thread
    m_expired = false;
    thread([this, interval, task]() {
        while (!m_tryToExpire)
        {
            // sleep every interval and do the task again and again until times up
//            this_thread::sleep_for(chrono::milliseconds(interval));  //There is an error of about 10 milliseconds
//            this_thread::sleep_for(chrono::microseconds(interval));  //There is an error of about 10 milliseconds
//            millSleep(interval);
            usSleep(interval);
            task();
        }

        {
            // timer be stopped, update the condition variable expired and wake main thread
            lock_guard<mutex> locker(m_mutex);
            m_expired = true;
            m_expiredCond.notify_one();
        }
    }).detach();
}

void Timer::stop()
{
    // do not stop again
    if (m_expired)
        return;

    if (m_tryToExpire)
        return;

    // wait until timer
    m_tryToExpire = true; // change this bool value to make timer while loop stop
    {
        unique_lock<mutex> locker(m_mutex);
        m_expiredCond.wait(locker, [this] {return m_expired == true; });

        // reset the timer
        if (m_expired == true)
            m_tryToExpire = false;
    }
}
```
