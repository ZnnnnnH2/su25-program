#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <iomanip>
using namespace std;

// 动作代码到名称的映射
map<int, string> action_names = {
    {0, "LEFT"}, {1, "UP"}, {2, "RIGHT"}, {3, "DOWN"}, {4, "SHIELD"}};

// 物品类型到名称的映射
map<int, string> item_names = {
    {1, "FOOD_1"}, {2, "FOOD_2"}, {3, "FOOD_3"}, {4, "FOOD_4"}, {5, "FOOD_5"}, {-1, "GROW_FOOD"}, {-2, "TRAP"}, {-3, "KEY"}, {-5, "CHEST"}};

struct GameState
{
    int tick;
    int pos_y, pos_x;
    int score;
    int length;
    int shield_cd;
    int shield_time;
    int candidates;
    string decision_type;
    string final_action;
    vector<string> items_considered;
    string target_info;
    string reason;
};

class LogAnalyzer
{
public:
    void analyze_log_line(const string &log_line)
    {
        GameState state;
        parse_log_line(log_line, state);
        print_analysis(state);
        cout << string(80, '=') << endl;
    }

    void analyze_log_file(const string &filename)
    {
        ifstream file(filename);
        string line;
        int turn = 1;

        cout << "🐍 贪吃蛇AI决策分析报告 🐍" << endl;
        cout << string(80, '=') << endl;

        while (getline(file, line))
        {
            if (!line.empty() && line[0] == '|')
            {
                cout << "回合 " << turn++ << ":" << endl;
                analyze_log_line(line);
            }
        }
    }

private:
    void parse_log_line(const string &log_line, GameState &state)
    {
        stringstream ss(log_line);
        string token;

        // 默认值
        state.tick = -1;
        state.pos_y = state.pos_x = -1;
        state.score = state.length = -1;
        state.shield_cd = state.shield_time = -1;
        state.candidates = 0;
        state.decision_type = "UNKNOWN";
        state.final_action = "UNKNOWN";

        while (getline(ss, token, '|'))
        {
            if (token.empty())
                continue;

            if (token.find("TICK:") == 0)
            {
                state.tick = stoi(token.substr(5));
            }
            else if (token.find("POS:") == 0)
            {
                size_t comma = token.find(',');
                if (comma != string::npos)
                {
                    state.pos_y = stoi(token.substr(4, comma - 4));
                    state.pos_x = stoi(token.substr(comma + 1));
                }
            }
            else if (token.find("SCORE:") == 0)
            {
                state.score = stoi(token.substr(6));
            }
            else if (token.find("LEN:") == 0)
            {
                state.length = stoi(token.substr(4));
            }
            else if (token.find("SHIELD_CD:") == 0)
            {
                state.shield_cd = stoi(token.substr(10));
            }
            else if (token.find("SHIELD_TIME:") == 0)
            {
                state.shield_time = stoi(token.substr(12));
            }
            else if (token.find("CAND:") == 0)
            {
                state.candidates = stoi(token.substr(5));
            }
            else if (token.find("ITEM_") == 0)
            {
                state.items_considered.push_back(token);
            }
            else if (token.find("CHEST@") == 0)
            {
                state.items_considered.push_back(token);
            }
            else if (token.find("TARGET@") == 0)
            {
                state.target_info = token;
            }
            else if (token.find("SURVIVAL_MODE") == 0)
            {
                state.decision_type = "SURVIVAL";
            }
            else if (token.find("NORMAL_MOVE:") == 0)
            {
                state.decision_type = "NORMAL";
                state.final_action = token.substr(12);
            }
            else if (token.find("SURVIVAL_MOVE:") == 0)
            {
                state.decision_type = "SURVIVAL";
                state.final_action = token.substr(14);
            }
            else if (token.find("SURVIVAL_SHIELD") == 0)
            {
                state.decision_type = "SURVIVAL";
                state.final_action = "4";
                state.reason = "无安全移动，开启护盾";
            }
            else if (token.find("DESPERATE_MOVE:") == 0)
            {
                state.decision_type = "DESPERATE";
                state.final_action = token.substr(15);
                state.reason = "冒险移动，忽略危险";
            }
            else if (token.find("FINAL_SHIELD") == 0)
            {
                state.decision_type = "FINAL";
                state.final_action = "4";
                state.reason = "绝望护盾，最后手段";
            }
            else if (token.find("DANGER_DETECTED") == 0)
            {
                state.reason = "检测到危险位置，激活护盾";
            }
            else if (token.find("SNAKE_PASS_WITH_SHIELD:") == 0)
            {
                state.decision_type = "SNAKE_PASS";
                state.final_action = token.substr(23);
                state.reason = "使用现有护盾穿过蛇身";
            }
            else if (token.find("SNAKE_DETECTED") == 0)
            {
                state.reason = "检测到蛇身阻挡，激活护盾";
                state.final_action = "4";
            }
            else if (token.find("ERROR:") == 0)
            {
                state.decision_type = "ERROR";
                state.reason = token;
            }
        }
    }

    void print_analysis(const GameState &state)
    {
        // 基本状态信息
        cout << "📍 位置状态:" << endl;
        cout << "   坐标: (" << state.pos_y << ", " << state.pos_x << ")" << endl;
        cout << "   分数: " << state.score << " | 长度: " << state.length << endl;

        // 护盾状态
        cout << "🛡️  护盾状态:" << endl;
        if (state.shield_time > 0)
        {
            cout << "   护盾激活中 (剩余 " << state.shield_time << " 回合)" << endl;
        }
        else if (state.shield_cd > 0)
        {
            cout << "   护盾冷却中 (剩余 " << state.shield_cd << " 回合)" << endl;
        }
        else
        {
            cout << "   护盾可用 ✅" << endl;
        }

        // 候选目标分析
        cout << "🎯 目标分析:" << endl;
        cout << "   发现 " << state.candidates << " 个候选目标" << endl;

        if (!state.items_considered.empty())
        {
            cout << "   考虑的物品:" << endl;
            for (const auto &item : state.items_considered)
            {
                analyze_item(item);
            }
        }

        if (!state.target_info.empty())
        {
            cout << "   选择的目标: ";
            analyze_target(state.target_info);
        }

        // 决策分析
        cout << "🧠 决策分析:" << endl;
        analyze_decision(state);

        // 最终行动
        cout << "⚡ 最终行动: ";
        if (state.final_action != "UNKNOWN")
        {
            int action_code = stoi(state.final_action);
            cout << action_names[action_code] << " (" << action_code << ")" << endl;
        }
        else
        {
            cout << "未知" << endl;
        }

        if (!state.reason.empty())
        {
            cout << "💭 决策原因: " << state.reason << endl;
        }
    }

    void analyze_item(const string &item_info)
    {
        if (item_info.find("ITEM_") == 0)
        {
            // 解析物品信息: ITEM_1@10,15:V6,D5,SC1.2
            size_t at_pos = item_info.find('@');
            size_t colon_pos = item_info.find(':');

            if (at_pos != string::npos && colon_pos != string::npos)
            {
                string type_str = item_info.substr(5, at_pos - 5);
                int item_type = stoi(type_str);

                string pos_str = item_info.substr(at_pos + 1, colon_pos - at_pos - 1);
                string stats_str = item_info.substr(colon_pos + 1);

                cout << "     " << item_names[item_type] << " @ " << pos_str;
                cout << " (" << stats_str << ")" << endl;
            }
        }
        else if (item_info.find("CHEST@") == 0)
        {
            cout << "     宝箱 " << item_info.substr(6) << endl;
        }
    }

    void analyze_target(const string &target_info)
    {
        // 解析目标信息: TARGET@10,15:SCORE:1.2,DIST:5,SNAKE_COST:0
        size_t at_pos = target_info.find('@');
        size_t colon_pos = target_info.find(':');

        if (at_pos != string::npos && colon_pos != string::npos)
        {
            string pos_str = target_info.substr(at_pos + 1, colon_pos - at_pos - 1);
            string stats_str = target_info.substr(colon_pos + 1);

            cout << "位置 " << pos_str << " (" << stats_str << ")" << endl;
        }
    }

    void analyze_decision(const GameState &state)
    {
        switch_decision_type(state.decision_type, state);
    }

    void switch_decision_type(const string &type, const GameState &state)
    {
        if (type == "NORMAL")
        {
            cout << "   决策类型: 🎯 正常目标追求" << endl;
            cout << "   AI找到了有价值的目标并规划了安全路径" << endl;
        }
        else if (type == "SURVIVAL")
        {
            cout << "   决策类型: 🆘 生存模式" << endl;
            cout << "   没有找到合适目标，AI进入求生状态" << endl;
            cout << "   优先寻找安全移动空间，避免死亡" << endl;
        }
        else if (type == "DESPERATE")
        {
            cout << "   决策类型: ⚠️  绝望移动" << endl;
            cout << "   情况危急，AI忽略部分安全检查，冒险移动" << endl;
        }
        else if (type == "FINAL")
        {
            cout << "   决策类型: 🚨 最终手段" << endl;
            cout << "   所有移动选项都不可行，强制尝试护盾" << endl;
        }
        else if (type == "SNAKE_PASS")
        {
            cout << "   决策类型: 🛡️  护盾穿越" << endl;
            cout << "   利用护盾穿过敌方蛇身，执行既定计划" << endl;
        }
        else if (type == "ERROR")
        {
            cout << "   决策类型: ❌ 错误处理" << endl;
            cout << "   算法遇到异常情况，启动错误恢复机制" << endl;
        }
        else
        {
            cout << "   决策类型: ❓ 未知状态" << endl;
        }
    }
};

int main(int argc, char *argv[])
{
    LogAnalyzer analyzer;

    if (argc > 1)
    {
        // 分析文件
        cout << "分析日志文件: " << argv[1] << endl;
        analyzer.analyze_log_file(argv[1]);
    }
    else
    {
        // 交互模式
        cout << "🐍 贪吃蛇AI日志分析器 🐍" << endl;
        cout << "请输入日志行 (以 | 开头), 或输入 'quit' 退出:" << endl;

        string line;
        while (getline(cin, line))
        {
            if (line == "quit" || line == "exit")
                break;
            if (!line.empty() && line[0] == '|')
            {
                analyzer.analyze_log_line(line);
            }
            else
            {
                cout << "请输入有效的日志行 (以 | 开头)" << endl;
            }
        }
    }

    return 0;
}
