#include "tile.h"
#include  <memory>

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
    return (int) layers.size() - currentLayer;
}

Tile::updateStatus Tile::update() noexcept {
    std::unique_ptr<Tile, std::function<void(Tile *)>> Resetter = {this, [](Tile *a) {
        a->running.store(false);
    }};

    int h, w, y;
    Complex diaganal, cor;
    QImage *img;
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
    unsigned char val;
    for (; y < h; y++) {
//        to the data of QImage no one can access until it is rendered or the tile is revoked
//        so we don't have to protect it with a mutex
        std::uint8_t *data = img->bits() + y * img->bytesPerLine();
        auto yy = (double) y / h * diaganal.imag() + cor.imag();
        for (int x = 0; x < w; x++) {
            auto xx = (double) x / w * diaganal.real() + cor.real();
            val = mand({xx, yy});
            data[x * 3 + 0] = 0;
            data[x * 3 + 1] = val;
            data[x * 3 + 2] = val;
        }
        {
            std::lock_guard lck(mut);
            if (revoked) {
                revoked = false;
                curY = y + 1;
                return status;
            }
        }
    }
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
