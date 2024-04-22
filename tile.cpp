#include "tile.h"
#include "tile_update.h"

Tile::Tile(int s) : rendered(nullptr) {
    for (int i = 2; i <= s; i *= 2) {
        layers.emplace_back(i, i, QImage::Format::Format_RGB888);
    }
}

void Tile::revoke(updateStatus st) {
    if (!running.load())
        return;
    std::unique_lock lck(mut);
    revoked = true;
    status = st;
}

void Tile::set(Complex cor, Complex diagg) noexcept {
    rendered.store(nullptr);
    std::unique_lock lck(mut);
    revoked = false;
    this->corner = cor;
    this->diag = diagg;
    currentLayer = -1;
    targetLayer = 0;
    curY = 0;
}


bool Tile::isLast() const noexcept {
    std::unique_lock lck(mut);
    return layers.size() == targetLayer;
}

int Tile::getPrior() const noexcept {
    std::unique_lock lck(mut);
    return (int)layers.size() - currentLayer;
}

Tile::updateStatus Tile::update() noexcept {
    std::unique_ptr<Tile, std::function<void(Tile*)>> Resetter = {
        this, [](Tile* a) {
            a->running.store(false);
        }
    };

    int h, w, y;
    Complex diaganal, cor;
    QImage* img;
    {
        std::unique_lock lck(mut);
        if (targetLayer == layers.size()) {
            curY = 0;
            return updateStatus::COMPLETED;
        }
        if (revoked) {
            revoked = false;
            return status;
        }
        img = &layers[targetLayer];
        h = img->height();
        w = img->width();
        diaganal = diag;
        cor = corner;
        y = curY;
    }

    // for (; y < h; y++) {
    do_update(img->bits(),
              h, w, y,
              img->bytesPerLine(),
              diaganal,
              cor);
    // {
    //     std::lock_guard lck(mut);
    //     if (revoked) {
    //         revoked = false;
    //         curY = y + 1;
    //         return status;
    //     }
    // }
    // }


    {
        rendered.store(&layers[targetLayer]);
        std::unique_lock lck(mut);
        curY = 0;
        currentLayer = targetLayer;
        targetLayer++;
        if (targetLayer == layers.size()) {
            return updateStatus::COMPLETED;
        }
        return updateStatus::UPDATED;
    }
}
