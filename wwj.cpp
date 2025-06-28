#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <thread>
#include <conio.h>
#include <windows.h>
#include <string>

using namespace std;

// 游戏常量
const int SCREEN_WIDTH = 80;
const int SCREEN_HEIGHT = 25;
const int TRACKS = 4;
const int PLAYER_START_X = 10;
const int PLAYER_START_Y = SCREEN_HEIGHT - 5;

// 游戏对象类型
enum ObjectType {
    EMPTY,
    PLAYER,
    BULLET,
    OBSTACLE,
    POWERUP_SPEED,
    POWERUP_DAMAGE
};

// 游戏对象类
class GameObject {
public:
    int x, y;
    ObjectType type;
    int health;
    int damage;
    
    GameObject(int x, int y, ObjectType type, int health = 1, int damage = 1) 
        : x(x), y(y), type(type), health(health), damage(damage) {}
};

// 游戏状态类
class GameState {
public:
    vector<vector<ObjectType>> screen;
    GameObject player;
    vector<GameObject> bullets;
    vector<GameObject> obstacles;
    vector<GameObject> powerups;
    
    int score;
    int gameTime;
    int bulletSpeed;
    int bulletDamage;
    bool gameOver;
    
    chrono::steady_clock::time_point startTime;
    chrono::steady_clock::time_point lastMoveTime;
    int moveCooldown; // 移动冷却时间（毫秒）
    
    GameState() : player(PLAYER_START_X, PLAYER_START_Y, PLAYER), 
                  score(0), gameTime(0), bulletSpeed(1), bulletDamage(1), gameOver(false),
                  moveCooldown(200) {
        screen.resize(SCREEN_HEIGHT, vector<ObjectType>(SCREEN_WIDTH, EMPTY));
        startTime = chrono::steady_clock::now();
        lastMoveTime = startTime;
    }
};

// 游戏类
class ShootingGame {
private:
    GameState gameState;
    
public:
    void run() {
        setupConsole();
        showInstructions();
        
        while (!gameState.gameOver) {
            updateGameTime();
            handleInput();
            updateGame();
            render();
            this_thread::sleep_for(chrono::milliseconds(100));
        }
        
        showGameOver();
    }
    
private:
    void setupConsole() {
        // 设置控制台窗口大小
        system("mode con cols=80 lines=25");
        system("title 射击游戏");
        
        // 隐藏光标
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_CURSOR_INFO cursorInfo;
        GetConsoleCursorInfo(hConsole, &cursorInfo);
        cursorInfo.bVisible = false;
        SetConsoleCursorInfo(hConsole, &cursorInfo);
    }
    
    void showInstructions() {
        system("cls");
        cout << "=== 射击游戏 ===" << endl;
        cout << "操作说明：" << endl;
        cout << "W/S 或 ↑/↓ : 上下移动" << endl;
        cout << "空格键: 暂停" << endl;
        cout << "ESC: 退出游戏" << endl;
        cout << endl;
        cout << "游戏规则：" << endl;
        cout << "- 在4条轨道间移动" << endl;
        cout << "- 自动射击摧毁障碍物" << endl;
        cout << "- 收集强化道具提升能力" << endl;
        cout << "- 避免碰撞障碍物" << endl;
        cout << endl;
        cout << "按任意键开始游戏...";
        _getch();
    }
    
    void updateGameTime() {
        auto currentTime = chrono::steady_clock::now();
        auto elapsed = chrono::duration_cast<chrono::seconds>(currentTime - gameState.startTime);
        gameState.gameTime = elapsed.count();
        gameState.score = gameState.gameTime;
    }
    
    void handleInput() {
        if (_kbhit()) {
            char key = _getch();
            switch (key) {
                case 'w':
                case 'W':
                case 72: // 上箭头
                    movePlayer(-1);
                    break;
                case 's':
                case 'S':
                case 80: // 下箭头
                    movePlayer(1);
                    break;
                case 27: // ESC
                    gameState.gameOver = true;
                    break;
            }
        }
    }
    
    void movePlayer(int direction) {
        auto currentTime = chrono::steady_clock::now();
        auto elapsed = chrono::duration_cast<chrono::milliseconds>(currentTime - gameState.lastMoveTime);
        
        if (elapsed.count() < gameState.moveCooldown) {
            return; // 还在冷却中，不允许移动
        }
        
        int newTrack = (gameState.player.y - (SCREEN_HEIGHT - 5)) / 2 + direction;
        if (newTrack >= 0 && newTrack < TRACKS) {
            gameState.player.y = SCREEN_HEIGHT - 5 + newTrack * 2;
            gameState.lastMoveTime = currentTime; // 记录移动时间
        }
    }
    
    void updateGame() {
        // 清空屏幕
        for (int y = 0; y < SCREEN_HEIGHT; y++) {
            for (int x = 0; x < SCREEN_WIDTH; x++) {
                gameState.screen[y][x] = EMPTY;
            }
        }
        
        // 更新玩家位置
        gameState.screen[gameState.player.y][gameState.player.x] = PLAYER;
        
        // 自动射击
        if (gameState.gameTime % (10 - gameState.bulletSpeed) == 0) {
            shoot();
        }
        
        // 更新子弹
        updateBullets();
        
        // 生成障碍物
        if (rand() % 20 == 0) {
            generateObstacle();
        }
        
        // 生成强化道具
        if (rand() % 100 == 0) {
            generatePowerup();
        }
        
        // 更新障碍物
        updateObstacles();
        
        // 更新强化道具
        updatePowerups();
        
        // 检测碰撞
        checkCollisions();
    }
    
    void shoot() {
        int bulletX = gameState.player.x + 1;
        int bulletY = gameState.player.y;
        gameState.bullets.push_back(GameObject(bulletX, bulletY, BULLET, 1, gameState.bulletDamage));
    }
    
    void updateBullets() {
        for (auto it = gameState.bullets.begin(); it != gameState.bullets.end();) {
            it->x += 2;
            gameState.screen[it->y][it->x] = BULLET;
            
            if (it->x >= SCREEN_WIDTH) {
                it = gameState.bullets.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    void generateObstacle() {
        int track = rand() % TRACKS;
        int y = SCREEN_HEIGHT - 5 + track * 2;
        int health = 1 + gameState.gameTime / 30; // 血量随时间增加
        gameState.obstacles.push_back(GameObject(SCREEN_WIDTH - 1, y, OBSTACLE, health, 1));
    }
    
    void generatePowerup() {
        int track = rand() % TRACKS;
        int y = SCREEN_HEIGHT - 5 + track * 2;
        ObjectType powerupType = (rand() % 2 == 0) ? POWERUP_SPEED : POWERUP_DAMAGE;
        gameState.powerups.push_back(GameObject(SCREEN_WIDTH - 1, y, powerupType, 1, 1));
    }
    
    void updateObstacles() {
        for (auto it = gameState.obstacles.begin(); it != gameState.obstacles.end();) {
            it->x -= 1;
            if (it->x < 0) {
                it = gameState.obstacles.erase(it);
            } else {
                gameState.screen[it->y][it->x] = OBSTACLE;
                ++it;
            }
        }
    }
    
    void updatePowerups() {
        for (auto it = gameState.powerups.begin(); it != gameState.powerups.end();) {
            it->x -= 1;
            if (it->x < 0) {
                it = gameState.powerups.erase(it);
            } else {
                gameState.screen[it->y][it->x] = it->type;
                ++it;
            }
        }
    }
    
    void checkCollisions() {
        // 检查玩家与障碍物碰撞
        for (const auto& obstacle : gameState.obstacles) {
            if (obstacle.x == gameState.player.x && obstacle.y == gameState.player.y) {
                gameState.gameOver = true;
                return;
            }
        }
        
        // 检查玩家与强化道具碰撞
        for (auto it = gameState.powerups.begin(); it != gameState.powerups.end();) {
            if (it->x == gameState.player.x && it->y == gameState.player.y) {
                if (it->type == POWERUP_SPEED) {
                    gameState.bulletSpeed = min(gameState.bulletSpeed + 1, 8);
                } else if (it->type == POWERUP_DAMAGE) {
                    gameState.bulletDamage = min(gameState.bulletDamage + 1, 5);
                }
                it = gameState.powerups.erase(it);
            } else {
                ++it;
            }
        }
        
        // 检查子弹与障碍物碰撞
        for (auto bulletIt = gameState.bullets.begin(); bulletIt != gameState.bullets.end();) {
            bool bulletHit = false;
            for (auto obstacleIt = gameState.obstacles.begin(); obstacleIt != gameState.obstacles.end();) {
                if (bulletIt->x == obstacleIt->x && bulletIt->y == obstacleIt->y) {
                    obstacleIt->health -= bulletIt->damage;
                    if (obstacleIt->health <= 0) {
                        obstacleIt = gameState.obstacles.erase(obstacleIt);
                    }
                    bulletHit = true;
                    break;
                } else {
                    ++obstacleIt;
                }
            }
            if (bulletHit) {
                bulletIt = gameState.bullets.erase(bulletIt);
            } else {
                ++bulletIt;
            }
        }
    }
    
    void render() {
        // 移动光标到屏幕顶部
        COORD coord;
        coord.X = 0;
        coord.Y = 0;
        SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
        
        // 渲染游戏界面
        cout << "=== 射击游戏 === 得分: " << gameState.score << " 时间: " << gameState.gameTime << "秒" << endl;
        cout << "子弹速度: " << gameState.bulletSpeed << " 子弹伤害: " << gameState.bulletDamage << endl;
        cout << string(SCREEN_WIDTH, '=') << endl;
        
        for (int y = 0; y < SCREEN_HEIGHT; y++) {
            for (int x = 0; x < SCREEN_WIDTH; x++) {
                switch (gameState.screen[y][x]) {
                    case EMPTY:
                        cout << " ";
                        break;
                    case PLAYER:
                        cout << "P";
                        break;
                    case BULLET:
                        cout << "|";
                        break;
                    case OBSTACLE:
                        cout << "#";
                        break;
                    case POWERUP_SPEED:
                        cout << "S";
                        break;
                    case POWERUP_DAMAGE:
                        cout << "D";
                        break;
                }
            }
            cout << endl;
        }
        
        cout << string(SCREEN_WIDTH, '=') << endl;
        cout << "操作: W/S移动 空格暂停 ESC退出" << endl;
    }
    
    void showGameOver() {
        system("cls");
        cout << "=== 游戏结束 ===" << endl;
        cout << "最终得分: " << gameState.score << endl;
        cout << "生存时间: " << gameState.gameTime << " 秒" << endl;
        cout << "按任意键退出...";
        _getch();
    }
};

int main() {
    srand(time(0));
    SetConsoleOutputCP(65001);
    ShootingGame game;
    game.run();
    
    return 0;
}
