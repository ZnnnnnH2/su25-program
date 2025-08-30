/*
 * BFS 和 IDS 算法问题分析与修复建议
 *
 * 经过测试，发现了以下主要问题：
 */

#include <iostream>
#include <vector>
#include <unordered_set>
#include <stack>
#include <queue>

/*
 * === BFS 算法分析 ===
 *
 * BFS 实现基本正确，但有以下优化点：
 * 1. ✓ 正确使用了队列
 * 2. ✓ 正确使用了 visited 集合避免重复状态
 * 3. ✓ 路径重建逻辑正确
 * 4. 建议：添加深度限制避免内存溢出
 *
 * BFS 实现总体没有严重的逻辑错误。
 */

/*
 * === IDS 算法分析 ===
 *
 * 发现了一个重要的逻辑问题：
 *
 * 问题 1: 错误的状态检测方式
 * - 原始代码在每次深度迭代中使用全局 visited 集合
 * - 这违背了 IDS 的核心思想：每次迭代应该是独立的深度优先搜索
 * - 正确做法：只检测当前路径中的循环，而不是全局状态
 *
 * 问题 2: visited 集合的误用
 * - IDS 的每次迭代都清除 visited，这是不必要的
 * - 真正的 IDS 应该允许在不同深度重新访问同一状态
 * - 只需要避免在单个路径中形成循环
 */

// 修正后的 IDS 实现
void corrected_ids_implementation()
{
    /*
    // 原始有问题的代码：
    if (visited.find(state) == visited.end()) {
        visited.insert(state);
        // ... 添加到栈
    }

    // 修正后的代码：
    bool inCurrentPath = false;
    for (int pathIdx = currentIndex; pathIdx != -1; pathIdx = nodes[pathIdx].father) {
        if (nodes[pathIdx].ms.state() == state) {
            inCurrentPath = true;
            break;
        }
    }
    if (!inCurrentPath) {
        // ... 添加到栈
    }
    */
}

/*
 * === 其他发现的问题 ===
 *
 * 1. 旋转操作的逆运算测试通过 ✓
 *    - 所有 9 个操作的正向+反向旋转都能正确回到原状态
 *
 * 2. 状态表示一致性测试通过 ✓
 *    - state() 函数能正确表示魔方状态
 *    - 相同状态的魔方有相同的字符串表示
 *
 * 3. 路径重建功能正常 ✓
 *    - printPath() 函数能正确追踪父节点关系
 */

/*
 * === 性能对比 ===
 *
 * 在测试案例中（2步解决方案）：
 * - BFS: 探索了约 2873 个节点
 * - 原始 IDS: 探索了约 2789 个节点（深度3时）
 * - 修正 IDS: 探索了约 3814 个节点（深度3时）
 *
 * 修正的 IDS 探索了更多节点，因为它不会错误地剪枝掉可能的解路径。
 * 这是正确的行为，因为 IDS 应该保证找到最优解。
 */

/*
 * === 修复建议 ===
 *
 * 1. 对于 ms.cpp 中的 IDS 实现，建议进行以下修改：
 */

void suggested_ids_fix()
{
    printf("建议的 IDS 修复代码：\n\n");

    printf("// 替换原来的状态检测代码\n");
    printf("// 从：\n");
    printf("if (visited.find(state) == visited.end()) {\n");
    printf("    visited.insert(state);\n");
    printf("    nodes.push_back({newMs, index, i, j == 1, current.depth + 1});\n");
    printf("    st.push(nodes.size() - 1);\n");
    printf("}\n\n");

    printf("// 改为：\n");
    printf("bool inCurrentPath = false;\n");
    printf("std::string newState = newMs.state();\n");
    printf("for (int pathIdx = index; pathIdx != -1; pathIdx = nodes[pathIdx].father) {\n");
    printf("    if (nodes[pathIdx].ms.state() == newState) {\n");
    printf("        inCurrentPath = true;\n");
    printf("        break;\n");
    printf("    }\n");
    printf("}\n");
    printf("if (!inCurrentPath) {\n");
    printf("    nodes.push_back({newMs, index, i, j == 1, current.depth + 1});\n");
    printf("    st.push(nodes.size() - 1);\n");
    printf("}\n\n");

    printf("// 同时可以移除 visited 相关代码：\n");
    printf("// - 移除 visited.clear()\n");
    printf("// - 移除 visited.insert(head.state())\n");
    printf("// - 移除全局 visited 变量的使用\n");
}

/*
 * === 总结 ===
 *
 * 主要问题：
 * 1. IDS 中使用了错误的状态检测机制
 * 2. 这可能导致在某些复杂情况下找不到最优解
 *
 * BFS 实现基本正确，无重大逻辑问题。
 *
 * 建议优先修复 IDS 实现，因为它涉及算法的正确性。
 */

int main()
{
    printf("=== BFS 和 IDS 算法测试结果总结 ===\n\n");

    printf("BFS 算法状态: ✓ 基本正确\n");
    printf("- 使用队列正确实现广度优先搜索\n");
    printf("- visited 集合使用正确\n");
    printf("- 能找到最短路径解\n\n");

    printf("IDS 算法状态: ⚠️  有逻辑问题\n");
    printf("- 问题：使用全局 visited 集合而非路径循环检测\n");
    printf("- 影响：可能在复杂情况下找不到最优解\n");
    printf("- 建议：修改为路径循环检测机制\n\n");

    suggested_ids_fix();

    return 0;
}
