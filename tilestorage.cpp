#include "tilestorage.h"

Tile_Storage::Tile_Storage() = default;


void Tile_Storage::revoke_Tiles() noexcept {
    for (auto& a : cache) {
        a.second->revoke();
        pool.push_back(a.second);
    }
    cache.clear();
}

void Tile_Storage::revoke_useless(int minX, int minY, int maxX, int maxY, int space) noexcept {
    for (auto& a : cache) {
        if (a.first.first < minX - space || a.first.first > maxX + space ||
            a.first.second < minY - space || a.first.second > maxY + space) {
            a.second->revoke();
            pool.push_back(a.second);
            cache.erase(a.first);
        }
    }
}

Tile* Tile_Storage::GetTile(int x, int y, Complex corner, Complex diag, int size) noexcept {
    auto iter = cache.lower_bound({x, y});
    if (iter != cache.end() && iter->first == Coord(x, y))
        return iter->second;

    Tile* ins = Get(size);
    ins->set(corner, diag);
    ins->revoke();
    cache.emplace(Coord(x, y), ins);
    return ins;
}

Tile* Tile_Storage::Get(int size) noexcept {
    if (!pool.empty()) {
        auto ret = pool.back();
        pool.pop_back();
        return ret;
    }
    return new Tile(size);
}

Tile_Storage::~Tile_Storage() {
    revoke_Tiles();
    for (auto const& a : pool)
        delete a;
}
