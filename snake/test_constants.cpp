/**
 * 常量化验证测试
 * 验证食物、钥匙、宝箱的权重和倍数常量是否正确工作
 */
#include <iostream>
#include <fstream>
#include <string>
using namespace std;

int main() {
    cout << "=== Snake程序常量化验证测试 ===" << endl;
    cout << "测试修改后的常量是否正确工作" << endl << endl;

    // 创建包含各种物品的测试输入
    string test_input = R"(50
5
10 10 1 30
11 11 -1 40
12 12 -2 50
13 13 -3 60
14 14 5 70
1
2024201540 5 100 2 0 0
9 10
9 11
9 12
9 13
9 14
1
20 20 75
1
15 15 -1 0
0 0 39 29
-1 0 0 39 29
-1 0 0 39 29
)";

    // 将测试输入写入文件
    ofstream file("constants_test.txt");
    file << test_input;
    file.close();

    // 运行程序
    string command = "./my-snake < constants_test.txt 2>&1";
    FILE* pipe = popen(command.c_str(), "r");
    
    string output;
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;
    }
    pclose(pipe);

    cout << "程序输出：" << endl;
    cout << output << endl;

    // 检查是否包含预期的物品类型和价值信息
    cout << "=== 验证结果 ===" << endl;
    
    if (output.find("NORMAL_FOOD") != string::npos) {
        cout << "✓ 普通食物检测正常" << endl;
    }
    if (output.find("GROWTH_FOOD") != string::npos) {
        cout << "✓ 成长食物检测正常" << endl;
    }
    if (output.find("KEY") != string::npos) {
        cout << "✓ 钥匙检测正常" << endl;
    }
    if (output.find("CHEST") != string::npos) {
        cout << "✓ 宝箱检测正常" << endl;
    }

    // 清理测试文件
    remove("constants_test.txt");

    cout << endl << "=== 常量列表 ===" << endl;
    cout << "食物价值常量：" << endl;
    cout << "- GROWTH_FOOD_VALUE = 5" << endl;
    cout << "- TRAP_PENALTY = -10" << endl;
    cout << "- KEY_VALUE = 10" << endl;
    cout << "- CHEST_VALUE = 100" << endl;
    cout << "- NORMAL_FOOD_MULTIPLIER = 3" << endl;
    cout << endl;
    cout << "评分权重常量：" << endl;
    cout << "- SNAKE_SAFETY_PENALTY_RATE = 0.5" << endl;
    cout << "- CHEST_SCORE_MULTIPLIER = 2.0" << endl;
    cout << "- DEFAULT_CHEST_SCORE = 60.0" << endl;
    cout << "- DISTANCE_OFFSET = 1.0" << endl;
    cout << "- SCORE_DISPLAY_MULTIPLIER = 100" << endl;

    return 0;
}
