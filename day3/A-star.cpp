#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <queue>
#include <vector>
#include <string>
#include <limits>

#define SIZE 22

// City name to index mapping
std::unordered_map<std::string, int> cityIndex;
std::vector<std::string> cityNames = {
    "Arad", "Zerind", "Timisoara", "Oradea", "Lugoj", "Mehadia",
    "Dobreta", "Craiova", "Rimnicu Vilcea", "Pitesti", "Sibiu",
    "Fagaras", "Bucharest", "Urziceni", "Giurgiu", "Hirsova",
    "Eforie", "Vaslui", "Iasi", "Neamt"};

int mp[SIZE][SIZE];    // adjacency matrix, real distance
int heuristic[SIZE];   // heuristic values (straight-line distance to Bucharest)
int start, goal;       // from Sibiu to Bucharest
std::vector<int> path; // store the path
int ans = std::numeric_limits<int>::max();

void input()
{
    // Initialize city index mapping
    for (int i = 0; i < cityNames.size(); i++)
    {
        cityIndex[cityNames[i]] = i;
    }

    // Initialize adjacency matrix
    for (int i = 0; i < SIZE; i++)
    {
        for (int j = 0; j < SIZE; j++)
        {
            mp[i][j] = (i == j) ? 0 : std::numeric_limits<int>::max();
        }
    }

    // Set up real distances (bidirectional)
    std::vector<std::tuple<std::string, std::string, int>> distances = {
        {"Arad", "Zerind", 75}, {"Arad", "Timisoara", 118}, {"Arad", "Sibiu", 140}, {"Zerind", "Oradea", 71}, {"Timisoara", "Lugoj", 111}, {"Lugoj", "Mehadia", 70}, {"Mehadia", "Dobreta", 75}, {"Dobreta", "Craiova", 120}, {"Craiova", "Rimnicu Vilcea", 146}, {"Craiova", "Pitesti", 138}, {"Sibiu", "Rimnicu Vilcea", 80}, {"Sibiu", "Fagaras", 99}, {"Rimnicu Vilcea", "Pitesti", 97}, {"Pitesti", "Bucharest", 101}, {"Fagaras", "Bucharest", 211}, {"Bucharest", "Urziceni", 85}, {"Bucharest", "Giurgiu", 90}, {"Urziceni", "Hirsova", 98}, {"Urziceni", "Vaslui", 142}, {"Hirsova", "Eforie", 86}, {"Vaslui", "Iasi", 92}, {"Iasi", "Neamt", 87}};

    for (auto &dist : distances)
    {
        int from = cityIndex[std::get<0>(dist)];
        int to = cityIndex[std::get<1>(dist)];
        int cost = std::get<2>(dist);
        mp[from][to] = cost;
        mp[to][from] = cost; // bidirectional
    }

    // Set up heuristic values (straight-line distance to Bucharest)
    std::vector<std::pair<std::string, int>> heuristicValues = {
        {"Arad", 366}, {"Bucharest", 0}, {"Craiova", 160}, {"Dobreta", 242}, {"Eforie", 161}, {"Fagaras", 176}, {"Giurgiu", 77}, {"Hirsova", 151}, {"Iasi", 226}, {"Lugoj", 244}, {"Mehadia", 241}, {"Neamt", 234}, {"Oradea", 380}, {"Pitesti", 10}, {"Rimnicu Vilcea", 193}, {"Sibiu", 233}, {"Timisoara", 329}, {"Urziceni", 80}, {"Vaslui", 199}, {"Zerind", 374}};

    for (auto &h : heuristicValues)
    {
        heuristic[cityIndex[h.first]] = h.second;
    }

    // Set start and goal
    start = cityIndex["Arad"];
    goal = cityIndex["Bucharest"];
}

void AStarSearch(int start, int goal)
{
    // Priority queue for A* search (min-heap based on f = g + h)
    std::priority_queue<std::tuple<int, int, int>, std::vector<std::tuple<int, int, int>>, std::greater<std::tuple<int, int, int>>> pq;

    std::vector<bool> visited(SIZE, false);
    std::vector<int> parent(SIZE, -1);
    std::vector<int> gCost(SIZE, std::numeric_limits<int>::max()); // Track the actual cost to reach each node

    // Start with the initial node
    gCost[start] = 0;
    pq.push({heuristic[start], 0, start});

    while (!pq.empty())
    {
        int currentHeuristic = std::get<0>(pq.top());
        int currentRealCost = std::get<1>(pq.top());
        int current = std::get<2>(pq.top());
        pq.pop();

        if (currentRealCost > ans)
            continue;

        if (visited[current])
            continue;

        visited[current] = true;

        if (current == goal)
        {
            while (current != start)
            {
                path.push_back(current);
                current = parent[current];
            }
            path.push_back(start);
            std::reverse(path.begin(), path.end());
            ans = currentRealCost;
            printf("ans: %d\n", ans);
            return; // Found the goal, return immediately
        }

        for (int neighbor = 0; neighbor < SIZE; neighbor++)
        {
            if (mp[current][neighbor] == std::numeric_limits<int>::max())
                continue; // No edge
            if (current == neighbor)
                continue;
            if (visited[neighbor])
                continue; // Skip already visited nodes

            int newCost = currentRealCost + mp[current][neighbor];

            // Only update if we found a better path to this neighbor
            if (newCost < gCost[neighbor])
            {
                gCost[neighbor] = newCost;
                parent[neighbor] = current;
                int newHeuristic = heuristic[neighbor] + newCost;
                if (newHeuristic < ans)
                {
                    pq.push({newHeuristic, newCost, neighbor});
                }
            }
        }
    }
}

void printPath()
{
    if (path.empty())
    {
        std::cout << "No path found from " << cityNames[start] << " to " << cityNames[goal] << std::endl;
        return;
    }

    std::cout << "Greedy Best-First Search Path:" << std::endl;
    std::cout << "Path: ";
    for (int i = 0; i < path.size(); i++)
    {
        std::cout << cityNames[path[i]];
        if (i < path.size() - 1)
        {
            std::cout << " -> ";
        }
    }
    std::cout << std::endl;

    std::cout << "Step-by-step details:" << std::endl;
    for (int i = 0; i < path.size() - 1; i++)
    {
        int from = path[i];
        int to = path[i + 1];
        std::cout << cityNames[from] << " -> " << cityNames[to]
                  << " (Real cost: " << mp[from][to]
                  << ", Heuristic: " << heuristic[to] << ")" << std::endl;
    }
}

int main()
{
    input();
    AStarSearch(start, goal);
    printPath();
    return 0;
}