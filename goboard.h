#ifndef GOBOARD_H
#define GOBOARD_H

#include <QMainWindow>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsItem>
#include <QMouseEvent>
#include <QVector>
#include <QMenuBar>
#include <QAction>
#include <QStatusBar>
#include <QLabel>

// 定义棋盘大小（19×19标准围棋）
const int BOARD_SIZE = 19;
// 定义每个交叉点的大小
const int CELL_SIZE = 30;
// 定义边缘留白
const int MARGIN = 30;
// 点击容错范围
const int CLICK_TOLERANCE = 60;

// 棋子颜色枚举
enum StoneColor {
    EMPTY,
    BLACK,
    WHITE
};

class GoBoard : public QMainWindow
{
    Q_OBJECT

public:
    GoBoard(QWidget *parent = nullptr);
    ~GoBoard();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void newGame();
    void undoMove();
    void aboutGame();

private:
    QGraphicsScene *scene;
    QGraphicsView *view;
    QStatusBar *statusBar;
    QLabel *blackCountLabel;
    QLabel *whiteCountLabel;

    // 存储棋盘状态
    StoneColor board[BOARD_SIZE][BOARD_SIZE];
    // 存储所有棋子的图形项
    QVector<QVector<QGraphicsEllipseItem*>> stones;

    // 当前回合（黑方先行）
    StoneColor currentPlayer;
    // 记录历史步骤，用于悔棋
    QVector<QPair<int, int>> moveHistory;
    // 记录每步棋的颜色，用于悔棋
    QVector<StoneColor> moveColors;
    // 记录每步提掉的棋子，用于悔棋
    QVector<QVector<QPair<int, int>>> capturedStonesHistory;
    // 棋子数量
    int blackCount;
    int whiteCount;

    // 打劫相关状态
    bool inKo;
    QPair<int, int> koPosition;

    // 初始化棋盘
    void initBoard();
    // 绘制棋盘
    void drawBoard();
    // 放置棋子
    bool placeStone(int row, int col, StoneColor color);
    // 检查落子是否有效
    bool isValidMove(int row, int col);
    // 检查是否有气
    bool hasLiberty(int row, int col, StoneColor color, QVector<QVector<bool>> &visited);
    // 提子，返回提掉的棋子数量
    int captureStones(int row, int col, StoneColor opponentColor, QVector<QPair<int, int>> &captured);
    // 更新状态栏
    void updateStatusBar();
    // 转换坐标
    bool convertPosToRowCol(const QPointF &scenePos, int &row, int &col);
};

#endif // GOBOARD_H
