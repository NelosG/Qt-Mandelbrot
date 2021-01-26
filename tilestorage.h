#pragma once

#include <QImage>

#include <map>
#include <vector>
#include <utility>
#include <complex>

#include "tile.h"

struct Tile_Storage {
    std::vector<Tile *> pool;
    using Coord = std::pair<int, int>;
    std::map<Coord, Tile *> cache;
    using Complex = Tile::Complex;

    Tile_Storage();

    ~Tile_Storage();

    void revoke_Tiles() noexcept;

    Tile *GetTile(int x, int y, Complex corner, Complex diag, int size) noexcept;

private:
    Tile *Get(int) noexcept;
};

