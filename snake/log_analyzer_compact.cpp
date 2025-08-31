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
    int tick = -1;
    int pos_y = -1, pos_x = -1;
    int score = -1;
    int length = -1;
    int shield_cd = -1;
    int shield_time = -1;
    int candidates = 0;
    string decision_type = "UNKNOWN";
    string final_action = "UNKNOWN";
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
        cout << string(50, '=') << endl;
    }

    void analyze_log_file(const string &filename)
    {
        ifstream file(filename);
        string content, line;
        int turn = 1;

        cout << "🐍 贪吃蛇AI决策分析报告 🐍" << endl;
        cout << string(50, '=') << endl;

        // 读取所有内容（处理无换行符的情况）
        while (getline(file, line))
        {
            content += line;
        }

        // 按 |T 分割识别回合
        vector<string> turns = split_turns(content);
        for (size_t i = 0; i < turns.size(); ++i)
        {
            if (turns[i].length() > 5)
            {
                cout << "回合 " << (i + 1) << ":" << endl;
                analyze_log_line(turns[i]);
            }
        }
    }

    void analyze_single_line(const string &log_line)
    {
        vector<string> turns = split_turns(log_line);
        for (size_t i = 0; i < turns.size(); ++i)
        {
            if (turns[i].length() > 5)
            {
                cout << "回合 " << (i + 1) << ":" << endl;
                analyze_log_line(turns[i]);
            }
        }
    }

private:
    vector<string> split_turns(const string &content)
    {
        vector<string> turns;
        size_t pos = 0;

        while ((pos = content.find("|T", pos)) != string::npos)
        {
            size_t end_pos = content.find("|T", pos + 1);
            if (end_pos == string::npos)
                end_pos = content.length();

            string turn_log = content.substr(pos, end_pos - pos);
            if (turn_log.length() > 5)
            {
                turns.push_back(turn_log);
            }
            pos = end_pos;
        }
        return turns;
    }

    void parse_log_line(const string &log_line, GameState &state)
    {
        stringstream ss(log_line);
        string token;

        while (getline(ss, token, '|'))
        {
            if (token.empty())
                continue;

            try
            {
                if (token[0] == 'T' && token.length() > 1)
                {
                    state.tick = stoi(token.substr(1));
                }
                else if (token[0] == 'P' && token.length() > 1)
                {
                    size_t comma = token.find(',');
                    if (comma != string::npos)
                    {
                        state.pos_y = stoi(token.substr(1, comma - 1));
                        state.pos_x = stoi(token.substr(comma + 1));
                    }
                }
                else if (token[0] == 'S' && token.length() > 1 && isdigit(token[1]))
                {
                    state.score = stoi(token.substr(1));
                }
                else if (token[0] == 'L' && token.length() > 1)
                {
                    state.length = stoi(token.substr(1));
                }
                else if (token.find("CD") == 0 && token.length() > 2)
                {
                    state.shield_cd = stoi(token.substr(2));
                }
                else if (token.find("ST") == 0 && token.length() > 2)
                {
                    state.shield_time = stoi(token.substr(2));
                }
                else if (token.find("CD:") == 0)
                {
                    state.candidates = stoi(token.substr(3));
                }
                else if (token[0] == 'I' && token.find('@') != string::npos)
                {
                    state.items_considered.push_back(token);
                }
                else if (token.find("C@") == 0)
                {
                    state.items_considered.push_back(token);
                }
                else if (token.find("TG@") == 0)
                {
                    state.target_info = token;
                }
                else if (token == "SUR")
                {
                    state.decision_type = "SURVIVAL";
                }
                else if (token.find("M:") == 0)
                {
                    state.decision_type = "NORMAL";
                    state.final_action = token.substr(2);
                }
                else if (token.find("SM:") == 0)
                {
                    state.decision_type = "SURVIVAL";
                    size_t comma = token.find(',');
                    if (comma != string::npos)
                    {
                        state.final_action = token.substr(3, comma - 3);
                        state.reason = "安全移动，可达性:" + token.substr(comma + 1);
                    }
                }
                else if (token == "SS")
                {
                    state.decision_type = "SURVIVAL";
                    state.final_action = "4";
                    state.reason = "生存护盾";
                }
                else if (token.find("DM:") == 0)
                {
                    state.decision_type = "DESPERATE";
                    state.final_action = token.substr(3);
                    state.reason = "绝望移动";
                }
                else if (token == "FS")
                {
                    state.decision_type = "FINAL";
                    state.final_action = "4";
                    state.reason = "最终护盾";
                }
                else if (token == "DNG")
                {
                    state.reason = "检测到危险";
                }
                else if (token.find("SP:") == 0)
                {
                    state.decision_type = "SNAKE_PASS";
                    state.final_action = token.substr(3);
                    state.reason = "护盾穿越";
                }
                else if (token == "SN")
                {
                    state.reason = "蛇身阻挡，激活护盾";
                    state.final_action = "4";
                }
                else if (token == "SB")
                {
                    state.reason = "蛇身阻挡，无护盾";
                }
                else if (token.find("ERR:") == 0)
                {
                    state.decision_type = "ERROR";
                    state.reason = token;
                }
            }
            catch (const exception &e)
            {
                // 忽略解析错误，继续处理下一个token
                continue;
            }
        }
    }

    void print_analysis(const GameState &state)
    {
        // 基本状态信息
        cout << "📍 位置:(" << state.pos_y << "," << state.pos_x << ") "
             << "分数:" << state.score << " 长度:" << state.length << endl;

        // 护盾状态
        cout << "🛡️ 护盾:";
        if (state.shield_time > 0)
        {
            cout << "激活(" << state.shield_time << "回合)";
        }
        else if (state.shield_cd > 0)
        {
            cout << "冷却(" << state.shield_cd << "回合)";
        }
        else
        {
            cout << "可用✅";
        }
        cout << endl;

        // 目标分析
        cout << "🎯 目标:" << state.candidates << "个候选";
        if (!state.target_info.empty())
        {
            cout << " 选择:" << analyze_target_brief(state.target_info);
        }
        cout << endl;

        // 决策分析
        cout << "🧠 决策:" << get_decision_brief(state.decision_type);
        if (!state.reason.empty())
        {
            cout << " (" << state.reason << ")";
        }
        cout << endl;

        // 最终行动
        cout << "⚡ 行动:";
        if (state.final_action != "UNKNOWN")
        {
            int action_code = stoi(state.final_action);
            cout << action_names[action_code];
        }
        else
        {
            cout << "未知";
        }
        cout << endl;
    }

    string analyze_target_brief(const string &target_info)
    {
        // 解析 TG@10,15:120,5 格式
        size_t at_pos = target_info.find('@');
        size_t colon_pos = target_info.find(':');

        if (at_pos != string::npos && colon_pos != string::npos)
        {
            string pos = target_info.substr(at_pos + 1, colon_pos - at_pos - 1);
            return pos;
        }
        return "未知";
    }

    string get_decision_brief(const string &type)
    {
        if (type == "NORMAL")
            return "🎯正常追求";
        else if (type == "SURVIVAL")
            return "🆘生存模式";
        else if (type == "DESPERATE")
            return "⚠️绝望移动";
        else if (type == "FINAL")
            return "🚨最终手段";
        else if (type == "SNAKE_PASS")
            return "🛡️护盾穿越";
        else if (type == "ERROR")
            return "❌错误处理";
        else
            return "❓未知";
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
        cout << "🐍 贪吃蛇AI日志分析器(精简版) 🐍" << endl;
        cout << "输入日志行或文件路径，'quit'退出:" << endl;

        string line;
        while (getline(cin, line))
        {
            if (line == "quit" || line == "exit")
                break;

            // 判断是文件路径还是日志内容
            if (line.find("|T") != string::npos)
            {
                analyzer.analyze_single_line(line);
            }
            else
            {
                cout << "请输入包含 |T 的有效日志" << endl;
            }
        }
    }

    return 0;
}
