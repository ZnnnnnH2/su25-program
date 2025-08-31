#include <bits/stdc++.h>
using namespace std;

/**
 * 贪吃蛇AI日志解析器
 * 用于解析和格式化输出贪吃蛇AI的决策日志
 */

struct LogEntry
{
    int tick = -1;
    string position;
    int score = -1;
    int length = -1;
    int shield_cd = -1;
    int shield_time = -1;

    vector<string> items;
    vector<string> chests;
    int candidates_count = -1;

    string mode;
    vector<string> moves;
    string chosen_move;

    string target;
    vector<string> path;
    string next_step;
    string action_type;
    string final_action;
};

class LogParser
{
private:
    vector<string> split(const string &str, char delimiter)
    {
        vector<string> tokens;
        stringstream ss(str);
        string token;

        while (getline(ss, token, delimiter))
        {
            if (!token.empty())
            {
                tokens.push_back(token);
            }
        }
        return tokens;
    }

    string extractValue(const string &token, const string &prefix)
    {
        if (token.find(prefix) == 0)
        {
            return token.substr(prefix.length());
        }
        return "";
    }

public:
    LogEntry parseLogLine(const string &logLine)
    {
        LogEntry entry;
        vector<string> tokens = split(logLine, '|');

        for (const auto &token : tokens)
        {
            if (token.empty())
                continue;

            // 基本状态信息
            string value = extractValue(token, "TICK:");
            if (!value.empty())
            {
                entry.tick = stoi(value);
                continue;
            }

            value = extractValue(token, "POSITION:");
            if (!value.empty())
            {
                entry.position = value;
                continue;
            }

            value = extractValue(token, "SCORE:");
            if (!value.empty())
            {
                entry.score = stoi(value);
                continue;
            }

            value = extractValue(token, "LENGTH:");
            if (!value.empty())
            {
                entry.length = stoi(value);
                continue;
            }

            value = extractValue(token, "SHIELD_COOLDOWN:");
            if (!value.empty())
            {
                entry.shield_cd = stoi(value);
                continue;
            }

            value = extractValue(token, "SHIELD_TIME:");
            if (!value.empty())
            {
                entry.shield_time = stoi(value);
                continue;
            }

            // 物品分析
            if (token == "ITEMS_ANALYSIS:")
            {
                entry.mode = "ITEMS_ANALYSIS";
                continue;
            }

            if (token.find("NORMAL_FOOD") == 0 || token.find("GROWTH_FOOD") == 0 ||
                token.find("KEY") == 0)
            {
                entry.items.push_back(token);
                continue;
            }

            if (token == "CHEST_ANALYSIS:")
            {
                continue;
            }

            if (token.find("CHEST@") == 0)
            {
                entry.chests.push_back(token);
                continue;
            }

            value = extractValue(token, "CANDIDATES_COUNT:");
            if (!value.empty())
            {
                entry.candidates_count = stoi(value);
                continue;
            }

            // 生存模式
            if (token == "SURVIVAL_MODE:")
            {
                entry.mode = "SURVIVAL";
                continue;
            }

            if (token == "SAFE_MOVE_ANALYSIS:")
            {
                continue;
            }

            // 移动分析
            if (token.find("LEFT@") == 0 || token.find("RIGHT@") == 0 ||
                token.find("UP@") == 0 || token.find("DOWN@") == 0)
            {
                entry.moves.push_back(token);
                continue;
            }

            if (token.find("SAFE_MOVE_CHOSEN:") == 0)
            {
                entry.chosen_move = token;
                entry.action_type = "SAFE_MOVE";
                continue;
            }

            if (token == "SHIELD_ACTIVATION:")
            {
                entry.action_type = "SHIELD_ACTIVATION";
                continue;
            }

            if (token == "DESPERATE_MOVE_ANALYSIS:")
            {
                entry.action_type = "DESPERATE_MOVE";
                continue;
            }

            if (token == "FORCED_SHIELD:")
            {
                entry.action_type = "FORCED_SHIELD";
                continue;
            }

            // 目标选择
            if (token.find("TARGET_SELECTED:") == 0)
            {
                entry.target = token;
                entry.mode = "TARGET_SELECTED";
                continue;
            }

            if (token == "PATH_BACKTRACK:")
            {
                continue;
            }

            if (token.find("(") != string::npos && token.find("<-(") != string::npos)
            {
                entry.path.push_back(token);
                continue;
            }

            if (token.find("NEXT_STEP:") == 0)
            {
                entry.next_step = token;
                continue;
            }

            // 特殊情况
            if (token == "DANGER_DETECTED:")
            {
                entry.action_type = "DANGER_DETECTED";
                continue;
            }

            if (token == "SNAKE_BODY_DETECTED:")
            {
                entry.action_type = "SNAKE_BODY_DETECTED";
                continue;
            }

            if (token.find("SHIELD_PASS:") == 0)
            {
                entry.final_action = token;
                entry.action_type = "SHIELD_PASS";
                continue;
            }

            if (token.find("SHIELD_PREPARE:") == 0)
            {
                entry.final_action = token;
                entry.action_type = "SHIELD_PREPARE";
                continue;
            }

            if (token.find("NORMAL_MOVE:") == 0)
            {
                entry.final_action = token;
                entry.action_type = "NORMAL_MOVE";
                continue;
            }

            // 错误情况
            if (token.find("ERROR:") == 0)
            {
                entry.action_type = "ERROR";
                entry.final_action = token;
                continue;
            }
        }

        return entry;
    }

    void printFormattedLog(const LogEntry &entry)
    {
        cout << "========================================\n";
        cout << "回合: " << entry.tick << "\n";
        cout << "位置: " << entry.position << "\n";
        cout << "分数: " << entry.score << " | 长度: " << entry.length << "\n";
        cout << "护盾: 冷却=" << entry.shield_cd << " 持续=" << entry.shield_time << "\n";
        cout << "----------------------------------------\n";

        if (entry.mode == "ITEMS_ANALYSIS")
        {
            cout << "模式: 物品分析\n";
            cout << "可用物品:\n";
            for (const auto &item : entry.items)
            {
                cout << "  " << item << "\n";
            }
            if (!entry.chests.empty())
            {
                cout << "可用宝箱:\n";
                for (const auto &chest : entry.chests)
                {
                    cout << "  " << chest << "\n";
                }
            }
            cout << "候选总数: " << entry.candidates_count << "\n";
        }

        if (entry.mode == "SURVIVAL")
        {
            cout << "模式: 生存模式 (未找到目标)\n";
            cout << "移动分析:\n";
            for (const auto &move : entry.moves)
            {
                cout << "  " << move << "\n";
            }
        }

        if (entry.mode == "TARGET_SELECTED")
        {
            cout << "模式: 目标已选择\n";
            cout << "目标: " << entry.target << "\n";
            if (!entry.path.empty())
            {
                cout << "路径回溯:\n";
                for (const auto &step : entry.path)
                {
                    cout << "  " << step << "\n";
                }
            }
            cout << "下一步: " << entry.next_step << "\n";
        }

        cout << "----------------------------------------\n";
        cout << "动作类型: " << entry.action_type << "\n";
        if (!entry.chosen_move.empty())
        {
            cout << "选择移动: " << entry.chosen_move << "\n";
        }
        if (!entry.final_action.empty())
        {
            cout << "最终动作: " << entry.final_action << "\n";
        }
        cout << "========================================\n\n";
    }

    void analyzeDecisionPattern(const vector<LogEntry> &entries)
    {
        cout << "\n=== 决策模式分析 ===\n";

        map<string, int> actionTypes;
        map<string, int> modes;
        int totalTicks = 0;
        int totalScore = 0;

        for (const auto &entry : entries)
        {
            if (entry.tick >= 0)
                totalTicks++;
            if (entry.score >= 0)
                totalScore = max(totalScore, entry.score);

            if (!entry.action_type.empty())
            {
                actionTypes[entry.action_type]++;
            }
            if (!entry.mode.empty())
            {
                modes[entry.mode]++;
            }
        }

        cout << "分析总回合数: " << totalTicks << "\n";
        cout << "最终分数: " << totalScore << "\n\n";

        cout << "动作类型分布:\n";
        for (const auto &[type, count] : actionTypes)
        {
            double percentage = (double)count / totalTicks * 100;
            cout << "  " << type << ": " << count << " (" << fixed << setprecision(1) << percentage << "%)\n";
        }

        cout << "\n模式分布:\n";
        for (const auto &[mode, count] : modes)
        {
            double percentage = (double)count / totalTicks * 100;
            cout << "  " << mode << ": " << count << " (" << fixed << setprecision(1) << percentage << "%)\n";
        }
    }

    void generateStatistics(const vector<LogEntry> &entries)
    {
        cout << "\n=== 详细统计信息 ===\n";

        if (entries.empty())
        {
            cout << "没有条目可分析。\n";
            return;
        }

        // 分数和长度统计
        vector<int> scores, lengths;
        for (const auto &entry : entries)
        {
            if (entry.score >= 0)
                scores.push_back(entry.score);
            if (entry.length >= 0)
                lengths.push_back(entry.length);
        }

        if (!scores.empty())
        {
            cout << "分数统计:\n";
            cout << "  最小值: " << *min_element(scores.begin(), scores.end()) << "\n";
            cout << "  最大值: " << *max_element(scores.begin(), scores.end()) << "\n";
            cout << "  平均值: " << fixed << setprecision(2)
                 << (double)accumulate(scores.begin(), scores.end(), 0) / scores.size() << "\n";
        }

        if (!lengths.empty())
        {
            cout << "长度统计:\n";
            cout << "  最小值: " << *min_element(lengths.begin(), lengths.end()) << "\n";
            cout << "  最大值: " << *max_element(lengths.begin(), lengths.end()) << "\n";
            cout << "  平均值: " << fixed << setprecision(2)
                 << (double)accumulate(lengths.begin(), lengths.end(), 0) / lengths.size() << "\n";
        }

        // 护盾使用统计
        int shield_active_count = 0, shield_cd_count = 0;
        for (const auto &entry : entries)
        {
            if (entry.shield_time > 0)
                shield_active_count++;
            if (entry.shield_cd > 0)
                shield_cd_count++;
        }

        cout << "护盾统计:\n";
        cout << "  护盾激活: " << shield_active_count << " 回合 ("
             << fixed << setprecision(1) << (double)shield_active_count / entries.size() * 100 << "%)\n";
        cout << "  护盾冷却: " << shield_cd_count << " 回合 ("
             << fixed << setprecision(1) << (double)shield_cd_count / entries.size() * 100 << "%)\n";

        // 候选目标统计
        vector<int> candidate_counts;
        for (const auto &entry : entries)
        {
            if (entry.candidates_count >= 0)
            {
                candidate_counts.push_back(entry.candidates_count);
            }
        }

        if (!candidate_counts.empty())
        {
            cout << "目标候选统计:\n";
            cout << "  最小值: " << *min_element(candidate_counts.begin(), candidate_counts.end()) << "\n";
            cout << "  最大值: " << *max_element(candidate_counts.begin(), candidate_counts.end()) << "\n";
            cout << "  平均值: " << fixed << setprecision(2)
                 << (double)accumulate(candidate_counts.begin(), candidate_counts.end(), 0) / candidate_counts.size() << "\n";
        }
    }
};

int main(int argc, char *argv[])
{
    LogParser parser;

    if (argc > 1)
    {
        // 从文件读取
        ifstream file(argv[1]);
        if (!file.is_open())
        {
            cerr << "错误: 无法打开文件 " << argv[1] << "\n";
            return 1;
        }

        string line;
        vector<LogEntry> entries;

        cout << "=== 解析日志文件: " << argv[1] << " ===\n\n";

        while (getline(file, line))
        {
            if (line.empty())
                continue;

            LogEntry entry = parser.parseLogLine(line);
            entries.push_back(entry);
            parser.printFormattedLog(entry);
        }

        parser.analyzeDecisionPattern(entries);
        parser.generateStatistics(entries);
        file.close();
    }
    else
    {
        // 从标准输入读取
        string line;
        vector<LogEntry> entries;

        cout << "=== 从标准输入读取 (输入空行结束) ===\n\n";

        while (getline(cin, line) && !line.empty())
        {
            LogEntry entry = parser.parseLogLine(line);
            entries.push_back(entry);
            parser.printFormattedLog(entry);
        }

        if (!entries.empty())
        {
            parser.analyzeDecisionPattern(entries);
            parser.generateStatistics(entries);
        }
    }

    return 0;
}
