#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include <ctime>
#include <set>
#include <windows.h>
using namespace std;

class Game24 {
private:
    vector<int> numbers;
    set<string> solutions;
    
    // 检查两个浮点数是否相等（考虑浮点误差）
    bool isEqual(double a, double b) {
        return abs(a - b) < 1e-10;
    }
    
    // 表达式结构体，记录值和表达式字符串
    struct Expr {
        double val;
        string str;
        Expr(double v, string s) : val(v), str(s) {}
    };
    
    // 递归生成所有可能的表达式
    void solve(vector<Expr> exprs, set<string>& solutions) {
        if (exprs.size() == 1) {
            if (fabs(exprs[0].val - 24) < 1e-6) {
                solutions.insert(exprs[0].str);
            }
            return;
        }
        for (size_t i = 0; i < exprs.size(); ++i) {
            for (size_t j = 0; j < exprs.size(); ++j) {
                if (i == j) continue;
                vector<Expr> next;
                for (size_t k = 0; k < exprs.size(); ++k) {
                    if (k != i && k != j) next.push_back(exprs[k]);
                }
                // 四则运算
                vector<pair<double, string>> candidates;
                candidates.push_back({exprs[i].val + exprs[j].val, "(" + exprs[i].str + "+" + exprs[j].str + ")"});
                candidates.push_back({exprs[i].val - exprs[j].val, "(" + exprs[i].str + "-" + exprs[j].str + ")"});
                candidates.push_back({exprs[i].val * exprs[j].val, "(" + exprs[i].str + "*" + exprs[j].str + ")"});
                if (fabs(exprs[j].val) > 1e-6) // 除数不能为0
                    candidates.push_back({exprs[i].val / exprs[j].val, "(" + exprs[i].str + "/" + exprs[j].str + ")"});
                for (auto& cand : candidates) {
                    next.push_back(Expr(cand.first, cand.second));
                    solve(next, solutions);
                    next.pop_back();
                }
            }
        }
    }

public:
    Game24() {
        srand(time(0));
    }
    
    // 生成新的4个数字
    void generateNumbers() {
        numbers.clear();
        solutions.clear();
        
        cout << "正在生成4个数字..." << endl;
        for (int i = 0; i < 4; i++) {
            numbers.push_back(rand() % 9 + 1); // 1-9的数字
        }
        
        cout << "生成的数字是: ";
        for (int num : numbers) {
            cout << num << " ";
        }
        cout << endl;
        
        // 预先计算解法但不显示
        calculateSolutions();
    }
    
    // 计算所有可能的24点解法（内部方法，不显示结果）
    void calculateSolutions() {
        solutions.clear();
        vector<Expr> exprs;
        for (int n : numbers) exprs.push_back(Expr(n, to_string(n)));
        solve(exprs, solutions);
    }
    
    // 显示所有解法
    void showSolutions() {
        cout << "找到 " << solutions.size() << " 种解法:" << endl;
        if (solutions.empty()) {
            cout << "没有找到24点的解法！" << endl;
        } else {
            int count = 1;
            for (const string& solution : solutions) {
                cout << count++ << ". " << solution << " = 24" << endl;
            }
        }
    }
    
    // 验证用户输入的表达式
    bool validateExpression(const string& expr) {
        // 这里简化验证，实际可以更复杂
        // 检查是否包含所有4个数字
        for (int num : numbers) {
            if (expr.find(to_string(num)) == string::npos) {
                return false;
            }
        }
        return true;
    }
    
    // 运行游戏
    void play() {
        cout << "=== 24点游戏 ===" << endl;
        cout << "规则：使用给定的4个数字，通过加减乘除运算得到24" << endl;
        cout << "输入 'new' 生成新数字，输入 'solve' 查看所有解法，输入 'quit' 退出" << endl;
        
        // 生成第一组数字
        generateNumbers();
        
        string input;
        while (true) {
            cout << "\n当前数字: ";
            for (int num : numbers) {
                cout << num << " ";
            }
            cout << endl;
            
            // 检查是否有解法
            if (solutions.empty()) {
                cout << "这组数字没有24点解法！正在生成新数字..." << endl;
                generateNumbers();
                continue;
            }
            
            cout << "请输入命令 (new/solve/quit) 或您的答案: ";
            getline(cin, input);
            
            if (input == "quit") {
                cout << "游戏结束，再见！" << endl;
                break;
            } else if (input == "new") {
                generateNumbers();
            } else if (input == "solve") {
                showSolutions();
            } else {
                if (validateExpression(input)) {
                    cout << "您的答案格式正确！" << endl;
                    cout << "提示：输入 'solve' 查看所有可能的解法" << endl;
                } else {
                    cout << "答案格式不正确，请确保使用了所有4个数字" << endl;
                }
            }
        }
    }
    
    // 直接测试模式
    void testMode() {
        cout << "=== 24点游戏测试模式 ===" << endl;
        generateNumbers();
        showSolutions();
    }
};

int main() {
    SetConsoleOutputCP(65001);
    Game24 game;
    
    cout << "选择模式：" << endl;
    cout << "1. 交互模式" << endl;
    cout << "2. 测试模式" << endl;
    cout << "请输入选择 (1 或 2): ";
    
    int choice;
    cin >> choice;
    cin.ignore(); // 清除换行符
    
    if (choice == 1) {
        game.play();
    } else if (choice == 2) {
        game.testMode();
    } else {
        cout << "无效选择，默认进入测试模式" << endl;
        game.testMode();
    }
    
    return 0;
}
