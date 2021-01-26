#include "mainwindow.h"
#include "ui_mainwindow.h"
//#include <iostream>

MainWindow::MainWindow(QWidget *parent)
        : QMainWindow(parent), ui(new Ui::MainWindow),
          threads(std::thread::hardware_concurrency() == 1 ? 1
                                                           : std::thread::hardware_concurrency() - 1) {
//          threads(11) {
    ui->setupUi(this);

    for (auto &a : threads) {
        a.second.m_W = this;
        a.first = std::thread(&threadData::threadFunc, &a.second);
    }
    double tmp = threads.size() == 1 ? std::max(width(), height()) : sqrt(width() * height() / threads.size()) + 1;

    size = 1;
    while (size < tmp) {
        size *= 2;
    }
    //magic with center setting
//    coordSystem.zero = {2. / threads.size() - 1,
//                        2. / threads.size() - 0.007 * threads.size()};
    some_tile.tile = new Tile();
    this->update();
}


void MainWindow::draw_Preview(QPainter &painter) {
    //quickly draw in the quality that allows performance
    int width = this->width();
    int height = this->height();

    auto *tile = some_tile.tile;
    auto &tx = some_tile.x;
    auto &ty = some_tile.y;
    auto &ts = some_tile.s;
    if (tx != coordSystem.xc || ty != coordSystem.yc || ts != coordSystem.scale) {
        if (min_layer_size != some_tile.min_size) {
            some_tile.tile->delete_layer();
            some_tile.tile->add_layer(min_layer_size);
            some_tile.s = min_layer_size;
        }
        Complex diag = Complex(width, height) * coordSystem.scale;
        Complex offset = coordSystem.zero - Complex(size + coordSystem.xc, size + coordSystem.yc) * coordSystem.scale;
        tile->set(offset, diag);
        //change the quality if we draw for too long (or too fast)
        timer.reset();
        tile->update();
        if (timer.elapsed() > 0.02) {
            if (min_layer_size / 2 >= 4)
                change_min_layer_size(-2);
        } else if (timer.elapsed() < 0.007) {
            if (min_layer_size * 2 <= std::min(width, height))
                change_min_layer_size(2);
        }

        tx = coordSystem.xc;
        ty = coordSystem.yc;
        ts = coordSystem.scale;
    }
    auto *img = tile->rendered.load();

    auto ratiow = (double) width / img->width();
    auto ratioh = (double) height / img->height();

    painter.setTransform(
            QTransform(
                    ratiow, 0,
                    0, ratioh,
                    0,
                    0));
    painter.drawImage(0, 0, *img);
}

void MainWindow::paintEvent(QPaintEvent *ev) {
    QMainWindow::paintEvent(ev);
    bool needsRerender = false;

    int width = this->width();
    int height = this->height();
    QPainter painter(this);
    int wh = std::max(width, height);
    int min_size = min_layer_size;

    draw_Preview(painter);


    int xoffset = coordSystem.xc % size;
    int yoffset = coordSystem.yc % size;
    xoffset += xoffset <= 0 ? size : 0;
    yoffset += yoffset <= 0 ? size : 0;

    priority_Tile prevTile = {1000, nullptr};

    for (int x = -size; x <= width; x += size) {
        int rx = x - coordSystem.xc;
        rx = rx >= 0 ? rx / size : (rx - size + 1) / size;
        rx *= size;


        for (int y = -size; y <= height; y += size) {
            int ry = y - coordSystem.yc;
            ry = ry >= 0 ? ry / size : (ry - size + 1) / size;
            ry *= size;
            auto *tile = tile_Storage.GetTile(rx, ry, Complex(rx, ry) * coordSystem.scale + coordSystem.zero,
                                              Complex(size, size) * coordSystem.scale, size);
            int cur_size = tile->get_cur_size();

            if (cur_size * 1000 / size >= min_size * 1000 / wh) {
                // to the rendered field in layers, no one else can change it without calling the set (which is only called by the mainwindow)
                QImage *tile_rendered = tile->rendered.load();
                if (tile_rendered != nullptr) {
                    auto ratiow = (double) size / tile_rendered->width();
                    auto ratioh = (double) size / tile_rendered->height();
                    painter.setTransform(
                            QTransform(
                                    ratiow, 0,
                                    0, ratioh,
                                    xoffset + x,
                                    yoffset + y));
                    painter.drawImage(0, 0, *tile_rendered);
                }

            } else {
                //skip generating tiles of worse quality than the one already rendered
                //otherwise, with good performance, you will still have to draw low-quality images
                if (!tile->running.load()) {
                    int c = 0;
                    while (cur_size * 1000 / size < min_size * 1000 / wh) {
                        cur_size *= 2;
                        c++;
                    }
                    tile->set_target_layer(c);
                }
            }
            if (!tile->isLast()) {
                needsRerender = true;
                if (!tile->running.load()) {
                    auto prior = tile->getPrior();
                    priority_Tile now = {prior, tile};
                    {
                        auto guard = std::lock_guard(mut);
                        tasks.push(now);
                    }
                    tile->running.store(true);
                    if (prevTile < now)
                        prevTile.tile->revoke(Tile::updateStatus::PAUSED);
                    prevTile = now;
                    cv.notify_one();
                }
            }
        }
    }
    if (needsRerender) {
        update();
    }
    painter.end();
    ev->accept();
}


void MainWindow::threadData::threadFunc() const {
    Tile::updateStatus status = Tile::updateStatus::COMPLETED;
    Tile *tile;
    while (running) {
        {
            auto lck = std::unique_lock(m_W->mut);
            if (status == Tile::updateStatus::UPDATED || status == Tile::updateStatus::PAUSED) {
                tile->running.store(true);
                m_W->tasks.push({tile->getPrior(), tile});
            } else {
                m_W->cv.wait(lck, [&]() {
                    return !(m_W->tasks.empty() && running);
                });
            }


            if (!running) {
                break;
            }
            tile = m_W->tasks.top().tile;
            m_W->tasks.pop();
        }
        status = tile->update();
    }
}


void MainWindow::wheelEvent(QWheelEvent *ev) {
    QMainWindow::wheelEvent(ev);
    //    if (mouse_Storage.pressed) {
    //        ev->ignore();
    //        return;
    //    }
    double prevScale = coordSystem.scale;
    double sc;
    if(!ev->angleDelta().isNull()) {
        sc = ev->angleDelta().ry() == 0 ? ev->angleDelta().rx() : ev->angleDelta().ry();

    } else if (!ev->pixelDelta().isNull()) {
        sc = ev->pixelDelta().ry() == 0 ?  ev->pixelDelta().rx() : ev->pixelDelta().ry();
    } else {
        ev->ignore();
        return;
    }

    coordSystem.scale *= std::pow(1.09, -sc / 101);

    int a = 1;
    if(ev->angleDelta().ry() > 0){
        a= -a;
    }
    coordSystem.xc += a*((int) ev->position().x() - width()/2)*0.2;
    coordSystem.yc += a*((int) ev->position().y() - height()/2)*0.2;

    coordSystem.zero -= prevScale * Complex(coordSystem.xc, coordSystem.yc);
    coordSystem.xc = coordSystem.yc = 0;
    tile_Storage.revoke_Tiles();
    update();
    ev->accept();
}

void MainWindow::mouseReleaseEvent(QMouseEvent *ev) {
    if (ev->button() == Qt::LeftButton) {
        mouse_Storage.pressed = false;
        ev->accept();
    } else {
        ev->ignore();
    }
}

void MainWindow::mousePressEvent(QMouseEvent *ev) {
    if (ev->button() == Qt::LeftButton) {
        mouse_Storage.pressed = true;
        mouse_Storage.lastX = ev->screenPos().x();
        mouse_Storage.lastY = ev->screenPos().y();
        ev->accept();
    } else {
        ev->ignore();
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *ev) {
    QMainWindow::mouseMoveEvent(ev);
    if (!mouse_Storage.pressed) {
        ev->ignore();
        return;
    }

    coordSystem.xc += ((int) ev->screenPos().x() - mouse_Storage.lastX);
    coordSystem.yc += ((int) ev->screenPos().y() - mouse_Storage.lastY);
    mouse_Storage.lastX = ev->screenPos().x();
    mouse_Storage.lastY = ev->screenPos().y();
    update();
    ev->accept();
}

void MainWindow::resizeEvent(QResizeEvent *ev) {
    QMainWindow::resizeEvent(ev);
    update();
    ev->accept();
}

MainWindow::~MainWindow() {
    delete some_tile.tile;
    for (auto &a : threads)
        a.second.running = false;
    cv.notify_all();
    for (auto &a : threads)
        a.first.join();
    delete ui;
}
