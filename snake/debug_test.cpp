#include <iostream>
using namespace std;

struct Safe
{
    int x_min, y_min, x_max, y_max;
};

static constexpr int SAFE_ZONE_SHRINK_THRESHOLD = 5;

inline int danger_safe_zone_debug(const Safe &current_safe_zone, const Safe &next_safe_zone,
                                  int next_shrink_tick, int current_tick, int y, int x)
{
    cout << "调试信息：" << endl;
    cout << "检测点: (" << x << "," << y << ")" << endl;
    cout << "当前安全区: [" << current_safe_zone.x_min << "," << current_safe_zone.x_max
         << "] x [" << current_safe_zone.y_min << "," << current_safe_zone.y_max << "]" << endl;
    cout << "下个安全区: [" << next_safe_zone.x_min << "," << next_safe_zone.x_max
         << "] x [" << next_safe_zone.y_min << "," << next_safe_zone.y_max << "]" << endl;

    // 不在当前安全区 - 立即危险
    bool in_current = (x >= current_safe_zone.x_min && x <= current_safe_zone.x_max &&
                       y >= current_safe_zone.y_min && y <= current_safe_zone.y_max);
    cout << "在当前安全区内: " << (in_current ? "是" : "否") << endl;

    if (!in_current)
        return -1;

    // 没有计划收缩 - 最低危险
    if (next_shrink_tick == -1)
    {
        cout << "无收缩计划" << endl;
        return 1;
    }

    int ticks_until_shrink = next_shrink_tick - current_tick;
    cout << "距离收缩还有: " << ticks_until_shrink << " 回合" << endl;

    // 短期内不会收缩 - 低危险
    if (ticks_until_shrink > SAFE_ZONE_SHRINK_THRESHOLD)
    {
        cout << "短期内不会收缩" << endl;
        return 2;
    }

    // 计算到收缩后安全区的曼哈顿"外距"（在内部为0）
    int dx = 0, dy = 0;
    if (x < next_safe_zone.x_min)
    {
        dx = next_safe_zone.x_min - x;
        cout << "x方向距离: " << x << " < " << next_safe_zone.x_min << ", dx = " << dx << endl;
    }
    else if (x > next_safe_zone.x_max)
    {
        dx = x - next_safe_zone.x_max;
        cout << "x方向距离: " << x << " > " << next_safe_zone.x_max << ", dx = " << dx << endl;
    }
    else
    {
        cout << "x在下个安全区内: " << next_safe_zone.x_min << " <= " << x << " <= " << next_safe_zone.x_max << endl;
    }

    if (y < next_safe_zone.y_min)
    {
        dy = next_safe_zone.y_min - y;
        cout << "y方向距离: " << y << " < " << next_safe_zone.y_min << ", dy = " << dy << endl;
    }
    else if (y > next_safe_zone.y_max)
    {
        dy = y - next_safe_zone.y_max;
        cout << "y方向距离: " << y << " > " << next_safe_zone.y_max << ", dy = " << dy << endl;
    }
    else
    {
        cout << "y在下个安全区内: " << next_safe_zone.y_min << " <= " << y << " <= " << next_safe_zone.y_max << endl;
    }

    int distance_to_next_zone = dx + dy;
    cout << "到下个安全区的距离: " << distance_to_next_zone << endl;

    // 危险度计算
    if (distance_to_next_zone == 0)
    {
        cout << "已在下个安全区内" << endl;
        if (ticks_until_shrink <= 1)
        {
            cout << "收缩临近，返回3" << endl;
            return 3;
        }
        else
        {
            cout << "安全，返回2" << endl;
            return 2;
        }
    }
    else
    {
        cout << "不在下个安全区内，计算高危险度" << endl;
        int time_pressure = max(1, SAFE_ZONE_SHRINK_THRESHOLD - ticks_until_shrink + 1);
        int result = min(15, 10 + distance_to_next_zone + time_pressure);
        cout << "时间压力: " << time_pressure << ", 最终危险度: " << result << endl;
        return result;
    }
}

int main()
{
    Safe current_zone = {10, 10, 30, 20};
    Safe next_zone = {15, 12, 25, 18};
    int shrink_tick = 100;

    cout << "测试点(20,15)在第99回合的详细分析:" << endl;
    int result = danger_safe_zone_debug(current_zone, next_zone, shrink_tick, 99, 15, 20);
    cout << "最终结果: " << result << endl;

    return 0;
}
