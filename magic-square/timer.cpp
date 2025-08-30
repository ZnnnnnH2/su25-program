#include <chrono>
#include <iostream>

class Timer
{
private:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = std::chrono::time_point<Clock>;

    TimePoint start_time;
    bool is_running;

public:
    // 构造函数，默认开始计时
    Timer() : is_running(true)
    {
        start_time = Clock::now();
    }

    // 重置计时器
    void reset()
    {
        start_time = Clock::now();
        is_running = true;
    }

    // 停止计时器并返回毫秒数
    double stop()
    {
        if (!is_running)
            return 0.0;

        auto end_time = Clock::now();
        is_running = false;

        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time);

        return duration.count();
    }

    // 停止计时器并返回秒数
    double stop_seconds()
    {
        if (!is_running)
            return 0.0;

        auto end_time = Clock::now();
        is_running = false;

        auto duration = std::chrono::duration_cast<std::chrono::duration<double>>(
            end_time - start_time);

        return duration.count();
    }
};
