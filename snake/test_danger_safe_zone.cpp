#include <iostream>
#include <cassert>
using namespace std;

// 简化的Safe结构用于测试
struct Safe
{
    int x_min, y_min, x_max, y_max;
};

// 从主文件中复制的常量
static constexpr int SAFE_ZONE_SHRINK_THRESHOLD = 5;

// 从主文件中复制的新函数
inline int danger_safe_zone(const Safe &current_safe_zone, const Safe &next_safe_zone,
                            int next_shrink_tick, int current_tick, int y, int x)
{
    // 不在当前安全区 - 立即危险
    if (!(x >= current_safe_zone.x_min && x <= current_safe_zone.x_max &&
          y >= current_safe_zone.y_min && y <= current_safe_zone.y_max))
        return -1;

    // 没有计划收缩 - 最低危险
    if (next_shrink_tick == -1)
    {
        return 1; // 稳定安全区的最小危险值
    }

    int ticks_until_shrink = next_shrink_tick - current_tick;

    // 短期内不会收缩 - 低危险
    if (ticks_until_shrink > SAFE_ZONE_SHRINK_THRESHOLD)
    {
        return 2; // 非收缩区域的低危险值
    }

    // 计算到收缩后安全区的曼哈顿"外距"（在内部为0）
    int dx = 0, dy = 0;
    if (x < next_safe_zone.x_min)
        dx = next_safe_zone.x_min - x;
    else if (x > next_safe_zone.x_max)
        dx = x - next_safe_zone.x_max;
    if (y < next_safe_zone.y_min)
        dy = next_safe_zone.y_min - y;
    else if (y > next_safe_zone.y_max)
        dy = y - next_safe_zone.y_max;

    int distance_to_next_zone = dx + dy;

    // 危险度计算
    if (distance_to_next_zone == 0)
    {
        // 已在下个安全区内 - 低危险
        if (ticks_until_shrink <= 1)
        {
            return 3; // 收缩临近时稍微提升的低危险
        }
        else
        {
            return 2; // 低危险 - 已在下个安全区内
        }
    }
    else
    {
        // 不在下个安全区内 - 高危险
        int time_pressure = max(1, SAFE_ZONE_SHRINK_THRESHOLD - ticks_until_shrink + 1);
        return min(15, 10 + distance_to_next_zone + time_pressure);
    }
}

int main()
{
    cout << "测试 danger_safe_zone 函数的任意时间点检测功能..." << endl;

    // 设置测试场景
    Safe current_zone = {10, 10, 30, 20}; // 当前安全区 (10,10) 到 (30,20)
    Safe next_zone = {15, 12, 25, 18};    // 下一个安全区 (15,12) 到 (25,18)
    int shrink_tick = 100;                // 第100回合收缩

    cout << "当前安全区: (" << current_zone.x_min << "," << current_zone.y_min
         << ") 到 (" << current_zone.x_max << "," << current_zone.y_max << ")" << endl;
    cout << "下一安全区: (" << next_zone.x_min << "," << next_zone.y_min
         << ") 到 (" << next_zone.x_max << "," << next_zone.y_max << ")" << endl;
    cout << "收缩时间: 第" << shrink_tick << "回合" << endl
         << endl;

    // 注意：函数参数顺序是 (current_zone, next_zone, shrink_tick, current_tick, y, x)

    // 测试用例1：不在当前安全区内的点
    int result1 = danger_safe_zone(current_zone, next_zone, shrink_tick, 95, 5, 5);
    cout << "测试1 - 点(5,5)在第95回合: 危险度 = " << result1;
    cout << " (期望 -1，不在当前安全区)" << endl;
    assert(result1 == -1);

    // 测试用例2：在当前安全区内，距离收缩还有很长时间
    int result2 = danger_safe_zone(current_zone, next_zone, shrink_tick, 90, 15, 20);
    cout << "测试2 - 点(20,15)在第90回合: 危险度 = " << result2;
    cout << " (期望 2，距离收缩还有10回合)" << endl;
    assert(result2 == 2);

    // 测试用例3：在当前安全区内，也在下个安全区内，接近收缩时间
    int result3 = danger_safe_zone(current_zone, next_zone, shrink_tick, 99, 15, 20);
    cout << "测试3 - 点(20,15)在第99回合: 危险度 = " << result3;
    cout << " (期望 3，即将收缩但已在下个安全区)" << endl;
    // 点(20,15)在next_zone(15,12,25,18)内，所以应该是低危险
    // 但由于只剩1回合，危险度会稍微增加
    cout << "  实际分析：x=20在[15,25]，y=15在[12,18]，在下个安全区内" << endl;
    assert(result3 == 3); // 修正后应该准确是3

    // 测试用例4：在当前安全区内，但不在下个安全区内，接近收缩时间
    int result4 = danger_safe_zone(current_zone, next_zone, shrink_tick, 97, 11, 11);
    cout << "测试4 - 点(11,11)在第97回合: 危险度 = " << result4;
    cout << " (期望高危险，不在下个安全区且接近收缩)" << endl;
    assert(result4 > 10);

    // 测试用例5：无收缩计划的情况
    int result5 = danger_safe_zone(current_zone, next_zone, -1, 95, 15, 20);
    cout << "测试5 - 点(20,15)无收缩计划: 危险度 = " << result5;
    cout << " (期望 1，无收缩计划)" << endl;
    assert(result5 == 1);

    // 测试用例6：检测不同时间点的危险变化
    cout << endl
         << "时间变化测试 - 同一点在不同时间的危险度变化:" << endl;
    for (int t = 90; t <= 100; t++)
    {
        int hazard = danger_safe_zone(current_zone, next_zone, shrink_tick, t, 11, 11);
        cout << "第" << t << "回合: 危险度 = " << hazard << endl;
    }

    cout << endl
         << "所有测试通过！danger_safe_zone 函数现在支持任意时间点的安全区域检测。" << endl;
    cout << "新功能特点:" << endl;
    cout << "1. 可以检测任意指定时间点的安全区危险度" << endl;
    cout << "2. 支持预测未来时间点的安全状况" << endl;
    cout << "3. 保持与原有接口的兼容性" << endl;
    cout << "4. 考虑时间压力和距离因素的动态危险评估" << endl;

    return 0;
}
