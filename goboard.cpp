#include "goboard.h"
#include <QGraphicsLineItem>
#include <QPen>
#include <QBrush>
#include <QMessageBox>
#include <QVector2D>
#include <QStatusBar>
#include <QLabel>
#include <QResizeEvent>
#include <QApplication>
#include <QQueue>
GoBoard::GoBoard(QWidget *parent)
    : QMainWindow(parent), currentPlayer(BLACK), blackCount(0), whiteCount(0), inKo(false)
{
    // 初始化棋盘状态
    for (int i = 0; i < BOARD_SIZE; ++i) {
        for (int j = 0; j < BOARD_SIZE; ++j) {
            board[i][j] = EMPTY;
        }
    }

    // 创建场景和视图
    scene = new QGraphicsScene(this);
    view = new QGraphicsView(scene, this);
    view->setRenderHint(QPainter::Antialiasing);
    view->setMinimumSize(BOARD_SIZE * CELL_SIZE + MARGIN * 2,
                         BOARD_SIZE * CELL_SIZE + MARGIN * 2);
    view->setAlignment(Qt::AlignCenter);
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setCentralWidget(view);

    // 创建菜单栏
    QMenu *gameMenu = menuBar()->addMenu("游戏");

    QAction *newAction = new QAction("新游戏", this);
    connect(newAction, &QAction::triggered, this, &GoBoard::newGame);
    gameMenu->addAction(newAction);

    QAction *undoAction = new QAction("悔棋", this);
    connect(undoAction, &QAction::triggered, this, &GoBoard::undoMove);
    gameMenu->addAction(undoAction);

    gameMenu->addSeparator();

    QAction *exitAction = new QAction("退出", this);
    connect(exitAction, &QAction::triggered, qApp, &QApplication::quit);
    gameMenu->addAction(exitAction);

    QMenu *helpMenu = menuBar()->addMenu("帮助");
    QAction *aboutAction = new QAction("关于", this);
    connect(aboutAction, &QAction::triggered, this, &GoBoard::aboutGame);
    helpMenu->addAction(aboutAction);

    // 初始化状态栏
    statusBar = new QStatusBar(this);
    setStatusBar(statusBar);
    blackCountLabel = new QLabel("黑棋: 0", this);
    whiteCountLabel = new QLabel("白棋: 0", this);
    statusBar->addWidget(blackCountLabel);
    statusBar->addWidget(new QLabel("  |  ", this));
    statusBar->addWidget(whiteCountLabel);
    statusBar->showMessage("黑棋先行");

    // 初始化棋盘
    initBoard();
    drawBoard();
}

GoBoard::~GoBoard()
{
    delete scene;
    // view由Qt自动管理
}

void GoBoard::initBoard()
{
    // 清除现有棋子
    for (auto &row : stones) {
        for (auto item : row) {
            if (item) {
                scene->removeItem(item);
                delete item;
            }
        }
    }
    stones.clear();

    // 重置棋盘状态
    for (int i = 0; i < BOARD_SIZE; ++i) {
        for (int j = 0; j < BOARD_SIZE; ++j) {
            board[i][j] = EMPTY;
        }
    }

    // 初始化棋子存储向量
    stones.resize(BOARD_SIZE);
    for (int i = 0; i < BOARD_SIZE; ++i) {
        stones[i].resize(BOARD_SIZE, nullptr);
    }

    currentPlayer = BLACK;
    moveHistory.clear();
    moveColors.clear();
    capturedStonesHistory.clear();
    blackCount = 0;
    whiteCount = 0;
    inKo = false;
    koPosition = QPair<int, int>(-1, -1);
    updateStatusBar();
}

void GoBoard::drawBoard()
{
    // 清除场景
    scene->clear();

    // 设置场景大小
    int sceneSize = qMin(width(), height()) - MARGIN * 2;
    int cellSize = sceneSize / (BOARD_SIZE - 1);

    scene->setSceneRect(0, 0,
                        BOARD_SIZE * cellSize + MARGIN * 2,
                        BOARD_SIZE * cellSize + MARGIN * 2);

    // 绘制棋盘背景
    QGraphicsRectItem *background = new QGraphicsRectItem(
        MARGIN - 10, MARGIN - 10,
        (BOARD_SIZE - 1) * cellSize + 20, (BOARD_SIZE - 1) * cellSize + 20);
    background->setBrush(QBrush(QColor(222, 184, 135))); // 木色
    scene->addItem(background);

    // 绘制网格线
    QPen pen(Qt::black, 1);

    // 横线
    for (int i = 0; i < BOARD_SIZE; ++i) {
        QGraphicsLineItem *line = new QGraphicsLineItem(
            MARGIN, MARGIN + i * cellSize,
            MARGIN + (BOARD_SIZE - 1) * cellSize, MARGIN + i * cellSize);
        line->setPen(pen);
        scene->addItem(line);
    }

    // 竖线
    for (int j = 0; j < BOARD_SIZE; ++j) {
        QGraphicsLineItem *line = new QGraphicsLineItem(
            MARGIN + j * cellSize, MARGIN,
            MARGIN + j * cellSize, MARGIN + (BOARD_SIZE - 1) * cellSize);
        line->setPen(pen);
        scene->addItem(line);
    }

    // 绘制天元和星位
    QVector<QPoint> starPoints;
    // 天元
    starPoints.append(QPoint(9, 9));
    // 星位
    starPoints.append(QPoint(3, 3));
    starPoints.append(QPoint(3, 9));
    starPoints.append(QPoint(3, 15));
    starPoints.append(QPoint(9, 3));
    starPoints.append(QPoint(9, 15));
    starPoints.append(QPoint(15, 3));
    starPoints.append(QPoint(15, 9));
    starPoints.append(QPoint(15, 15));

    foreach (QPoint p, starPoints) {
        QGraphicsEllipseItem *star = new QGraphicsEllipseItem(
            MARGIN + p.x() * cellSize - 3,
            MARGIN + p.y() * cellSize - 3,
            6, 6);
        star->setBrush(QBrush(Qt::black));
        scene->addItem(star);
    }
}

void GoBoard::mousePressEvent(QMouseEvent *event)
{
    // 将鼠标点击位置转换为场景坐标
    QPointF scenePos = view->mapToScene(event->pos());

    // 计算落子位置（行和列）
    int row, col;
    if (convertPosToRowCol(scenePos, row, col)) {
        // 尝试放置棋子
        if (placeStone(row, col, currentPlayer)) {
            // 切换玩家
            currentPlayer = (currentPlayer == BLACK) ? WHITE : BLACK;
            updateStatusBar();
        }
    }

    QMainWindow::mousePressEvent(event);
}

bool GoBoard::convertPosToRowCol(const QPointF &scenePos, int &row, int &col)
{
    int sceneSize = qMin(width(), height()) - MARGIN * 2;
    int cellSize = sceneSize / (BOARD_SIZE - 1);

    // 计算最近的交叉点
    col = qRound((scenePos.x() - MARGIN) / cellSize);
    row = qRound((scenePos.y() - MARGIN) / cellSize-1);

    // 计算与交叉点的距离，检查是否在容错范围内
    qreal xDist = qAbs(scenePos.x() - (MARGIN + col * cellSize));
    qreal yDist = qAbs(scenePos.y() - (MARGIN + row * cellSize));

    if (xDist > CLICK_TOLERANCE || yDist > CLICK_TOLERANCE) {
        return false;
    }

    // 检查是否在棋盘范围内
    if (row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE) {
        return true;
    }

    return false;
}

bool GoBoard::placeStone(int row, int col, StoneColor color)
{
    // 检查该位置是否已有棋子
    if (board[row][col] != EMPTY) {
        statusBar->showMessage("该位置已有棋子！");
        return false;
    }

    // 检查是否在打劫状态且尝试落子在打劫位置
    if (inKo && row == koPosition.first && col == koPosition.second) {
        statusBar->showMessage("打劫！不能立即提回，请先下在其他位置。");
        return false;
    }

    // 检查落子是否有效
    if (!isValidMove(row, col)) {
        statusBar->showMessage("这个位置不能落子，没有气且不能提子！");
        return false;
    }

    // 记录此步提掉的棋子
    QVector<QPair<int, int>> capturedThisMove;

    // 临时放置棋子，用于检查提子
    board[row][col] = color;

    // 提掉没有气的对方棋子
    StoneColor opponent = (color == BLACK) ? WHITE : BLACK;
    int capturedCount = captureStones(row, col, opponent, capturedThisMove);

    // 检查是否形成打劫
    inKo = false;
    koPosition = QPair<int, int>(-1, -1);
    if (capturedCount == 1 && capturedThisMove.size() == 1) {
        // 检查刚下的这颗棋是否只有一口气，且周围只有一个对方棋子
        QVector<QVector<bool>> visited(BOARD_SIZE, QVector<bool>(BOARD_SIZE, false));
        if (!hasLiberty(row, col, color, visited)) {
            // 形成打劫
            inKo = true;
            koPosition = capturedThisMove[0];
        }
    }

    // 创建棋子图形项
    int sceneSize = qMin(width(), height()) - MARGIN * 2;
    int cellSize = sceneSize / (BOARD_SIZE - 1);

    QGraphicsEllipseItem *stone = new QGraphicsEllipseItem(
        MARGIN + col * cellSize - cellSize / 2 + 1,
        MARGIN + row * cellSize - cellSize / 2 + 1,
        cellSize - 2, cellSize - 2);

    stone->setBrush(QBrush(color == BLACK ? Qt::black : Qt::white));
    stone->setPen(QPen(color == BLACK ? Qt::darkGray : Qt::lightGray, 1));
    scene->addItem(stone);

    stones[row][col] = stone;

    // 更新棋子计数
    if (color == BLACK) {
        blackCount++;
        whiteCount -= capturedCount;
    } else {
        whiteCount++;
        blackCount -= capturedCount;
    }

    // 记录历史
    moveHistory.append(QPair<int, int>(row, col));
    moveColors.append(color);
    capturedStonesHistory.append(capturedThisMove);

    return true;
}

bool GoBoard::isValidMove(int row, int col)
{
    // 简单规则检查：
    // 1. 该位置必须为空
    if (board[row][col] != EMPTY) {
        return false;
    }

    // 2. 临时放置棋子，检查是否有气或能提子
    StoneColor original = board[row][col];
    board[row][col] = currentPlayer;

    // 检查是否有气
    QVector<QVector<bool>> visited(BOARD_SIZE, QVector<bool>(BOARD_SIZE, false));
    bool has_liberty = hasLiberty(row, col, currentPlayer, visited);

    // 如果没有气，检查是否能提掉对方棋子
    if (!has_liberty) {
        StoneColor opponent = (currentPlayer == BLACK) ? WHITE : BLACK;
        bool can_capture = false;

        // 检查周围四个方向的对方棋子
        const int dx[] = {-1, 1, 0, 0};
        const int dy[] = {0, 0, -1, 1};

        for (int i = 0; i < 4; ++i) {
            int r = row + dx[i];
            int c = col + dy[i];

            if (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE &&
                board[r][c] == opponent) {

                QVector<QVector<bool>> opp_visited(BOARD_SIZE, QVector<bool>(BOARD_SIZE, false));
                if (!hasLiberty(r, c, opponent, opp_visited)) {
                    can_capture = true;
                    break;
                }
            }
        }

        has_liberty = can_capture;
    }

    // 恢复棋盘状态
    board[row][col] = original;

    return has_liberty;
}

bool GoBoard::hasLiberty(int row, int col, StoneColor color, QVector<QVector<bool>> &visited)
{
    // 检查是否越界
    if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE) {
        return false;
    }

    // 检查是否已访问
    if (visited[row][col]) {
        return false;
    }

    // 标记为已访问
    visited[row][col] = true;

    // 如果是空格，说明有气
    if (board[row][col] == EMPTY) {
        return true;
    }

    // 如果不是同一颜色的棋子，返回false
    if (board[row][col] != color) {
        return false;
    }

    // 检查四个方向
    const int dx[] = {-1, 1, 0, 0};
    const int dy[] = {0, 0, -1, 1};

    for (int i = 0; i < 4; ++i) {
        if (hasLiberty(row + dx[i], col + dy[i], color, visited)) {
            return true;
        }
    }

    return false;
}

int GoBoard::captureStones(int row, int col, StoneColor opponentColor, QVector<QPair<int, int>> &captured)
{
    int capturedCount = 0;
    const int dx[] = {-1, 1, 0, 0}; // 上下左右四个方向
    const int dy[] = {0, 0, -1, 1};

    // 标记已检查的棋子，避免重复处理
    QVector<QVector<bool>> checked(BOARD_SIZE, QVector<bool>(BOARD_SIZE, false));

    // 检查落子位置周围的四个方向
    for (int i = 0; i < 4; ++i) {
        int x = row + dx[i];
        int y = col + dy[i];

        // 边界检查 + 确认是对方棋子 + 未检查过
        if (x < 0 || x >= BOARD_SIZE || y < 0 || y >= BOARD_SIZE)
            continue;
        if (board[x][y] != opponentColor || checked[x][y])
            continue;

        // 步骤1：找到当前位置相连的所有对方棋子（形成气团）
        QVector<QPair<int, int>> group; // 存储气团所有棋子
        QQueue<QPair<int, int>> queue;  // BFS队列
        queue.enqueue({x, y});
        checked[x][y] = true;
        group.append({x, y});

        // BFS遍历所有相连的对方棋子
        while (!queue.isEmpty()) {
            auto current = queue.dequeue();
            int cx = current.first;
            int cy = current.second;

            // 检查当前棋子的四个方向
            for (int j = 0; j < 4; ++j) {
                int nx = cx + dx[j];
                int ny = cy + dy[j];
                if (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE &&
                    board[nx][ny] == opponentColor && !checked[nx][ny]) {
                    checked[nx][ny] = true;
                    queue.enqueue({nx, ny});
                    group.append({nx, ny});
                }
            }
        }

        // 步骤2：检查该气团是否有气
        QVector<QVector<bool>> visited(BOARD_SIZE, QVector<bool>(BOARD_SIZE, false));
        bool hasLib = false;
        for (auto &pos : group) {
            if (hasLiberty(pos.first, pos.second, opponentColor, visited)) {
                hasLib = true;
                break;
            }
        }

        // 步骤3：若气团无气，则提掉所有棋子
        if (!hasLib) {
            for (auto &pos : group) {
                int rx = pos.first;
                int ry = pos.second;

                // 从棋盘状态中移除
                board[rx][ry] = EMPTY;

                // 从图形界面中移除
                if (stones[rx][ry]) {
                    scene->removeItem(stones[rx][ry]);
                    delete stones[rx][ry];
                    stones[rx][ry] = nullptr;
                }

                // 记录被提的棋子
                captured.append({rx, ry});
                capturedCount++;
            }
        }
    }

    return capturedCount;
}

void GoBoard::updateStatusBar()
{
    blackCountLabel->setText(QString("黑棋: %1").arg(blackCount));
    whiteCountLabel->setText(QString("白棋: %1").arg(whiteCount));
    statusBar->showMessage(currentPlayer == BLACK ? "黑棋回合" : "白棋回合");
}

void GoBoard::resizeEvent(QResizeEvent *event)
{
    // 窗口大小改变时重新绘制棋盘
    drawBoard();
    // 重新绘制所有棋子
    int sceneSize = qMin(width(), height()) - MARGIN * 2;
    int cellSize = sceneSize / (BOARD_SIZE - 1);

    for (int i = 0; i < BOARD_SIZE; ++i) {
        for (int j = 0; j < BOARD_SIZE; ++j) {
            if (board[i][j] != EMPTY) {
                if (stones[i][j]) {
                    scene->removeItem(stones[i][j]);
                    delete stones[i][j];
                }

                QGraphicsEllipseItem *stone = new QGraphicsEllipseItem(
                    MARGIN + j * cellSize - cellSize / 2 + 1,
                    MARGIN + i * cellSize - cellSize / 2 + 1,
                    cellSize - 2, cellSize - 2);

                stone->setBrush(QBrush(board[i][j] == BLACK ? Qt::black : Qt::white));
                stone->setPen(QPen(board[i][j] == BLACK ? Qt::darkGray : Qt::lightGray, 1));
                scene->addItem(stone);

                stones[i][j] = stone;
            }
        }
    }

    QMainWindow::resizeEvent(event);
}

void GoBoard::newGame()
{
    if (QMessageBox::question(this, "新游戏", "确定要开始新游戏吗？当前进度将丢失。") == QMessageBox::Yes) {
        initBoard();
        drawBoard();
    }
}

void GoBoard::undoMove()
{
    if (moveHistory.isEmpty()) {
        QMessageBox::information(this, "悔棋", "没有可悔的步骤！");
        return;
    }

    // 取出最后一步
    QPair<int, int> lastMove = moveHistory.takeLast();
    int row = lastMove.first;
    int col = lastMove.second;

    // 取出最后一步的颜色
    StoneColor lastColor = moveColors.takeLast();

    // 取出这步提掉的棋子
    QVector<QPair<int, int>> captured = capturedStonesHistory.takeLast();

    // 恢复被提掉的棋子
    StoneColor opponent = (lastColor == BLACK) ? WHITE : BLACK;
    for (auto &pos : captured) {
        int x = pos.first;
        int y = pos.second;

        // 恢复棋盘状态
        board[x][y] = opponent;

        // 重新创建棋子图形项
        int sceneSize = qMin(width(), height()) - MARGIN * 2;
        int cellSize = sceneSize / (BOARD_SIZE - 1);

        QGraphicsEllipseItem *stone = new QGraphicsEllipseItem(
            MARGIN + y * cellSize - cellSize / 2 + 1,
            MARGIN + x * cellSize - cellSize / 2 + 1,
            cellSize - 2, cellSize - 2);

        stone->setBrush(QBrush(opponent == BLACK ? Qt::black : Qt::white));
        stone->setPen(QPen(opponent == BLACK ? Qt::darkGray : Qt::lightGray, 1));
        scene->addItem(stone);

        stones[x][y] = stone;
    }

    // 移除最后一步放置的棋子
    if (stones[row][col]) {
        scene->removeItem(stones[row][col]);
        delete stones[row][col];
        stones[row][col] = nullptr;
    }

    // 恢复棋盘状态
    board[row][col] = EMPTY;

    // 更新棋子计数
    if (lastColor == BLACK) {
        blackCount--;
        whiteCount += captured.size();
    } else {
        whiteCount--;
        blackCount += captured.size();
    }

    // 重置打劫状态
    inKo = false;
    koPosition = QPair<int, int>(-1, -1);

    // 切换回上一个玩家
    currentPlayer = lastColor;
    updateStatusBar();
}

void GoBoard::aboutGame()
{
    QMessageBox::about(this, "关于围棋", "围棋是一种源于中国的古老棋类游戏，\n"
                                         "在19×19的棋盘上进行，黑白双方轮流落子。\n"
                                         "目标是围出更多的空，并提掉对方的棋子。");
}
