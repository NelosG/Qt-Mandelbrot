#pragma once

#include <complex>
#include <queue>
#include <mutex>
#include <utility>
#include <thread>
#include <vector>
#include <condition_variable>

#include <QMainWindow>
#include <QPaintEvent>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QWidget>
#include <QResizeEvent>

#include "tile.h"
#include "tilestorage.h"
#include "timer.h"

QT_BEGIN_NAMESPACE
namespace Ui {
    class MainWindow;
}
QT_END_NAMESPACE


class MainWindow : public QMainWindow {
Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

    ~MainWindow() override;

    void paintEvent(QPaintEvent*) override;

    void mouseReleaseEvent(QMouseEvent*) override;

    void mousePressEvent(QMouseEvent*) override;

    void mouseMoveEvent(QMouseEvent*) override;

    void wheelEvent(QWheelEvent*) override;

    void resizeEvent(QResizeEvent* ev) override;

    void draw_Preview(QPainter&);

    using Complex = Tile::Complex;

private:
    Ui::MainWindow* ui;

    struct {
        Tile* tile = nullptr;
        int x = 0, y = 0, min_size = 0;
        long double s = 0.;
    } some_tile;

    int min_layer_size = 64;

    int size = -1;

    Timer timer;

    void change_min_layer_size(char a) {
        if (a == 0) {
            exit(-31);
        }
        min_layer_size = a < 0 ? min_layer_size / (-a) : min_layer_size * a;
        if(min_layer_size > size) {
            min_layer_size = size;
        }
    }

    Tile_Storage tile_Storage;

    std::priority_queue<priority_Tile> tasks;
    std::mutex mut;
    std::condition_variable cv;


    struct threadData {
        //changes the value only in the destructor therefore we do not protect
        bool running = true;

        MainWindow* m_W = nullptr;

        void threadFunc() const;
    };

    std::vector<std::pair<std::thread, threadData>> threads;

    struct {
        Complex zero = {0, 0};
        long double scale = 1. / 256;
        int xc = 0, yc = 0;
    } coordSystem;

    struct {
        bool pressed = false;
        int lastX = 0;
        int lastY = 0;
    } mouse_Storage;
};
