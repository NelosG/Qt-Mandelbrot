#pragma once

#include <complex>
#include <mutex>
#include <vector>
#include <atomic>
#include <cstdint>

#include <QImage>

class Tile {
public:
    std::atomic<QImage*> rendered;
    std::atomic<bool> running = false;

    enum class updateStatus {
        UPDATED,
        STOPPED,
        PAUSED,
        COMPLETED
    };
    unsigned int targetLayer = 0;
    using Complex = std::complex<long double>;

private:
    int currentLayer = -1;
    std::vector<QImage> layers;
    int curY = 0;
    Complex corner, diag;
    mutable std::mutex mut;

    bool revoked = false;
    updateStatus status = updateStatus::STOPPED;

    static std::uint8_t mand(Complex c) noexcept {
        Complex z = {0, 0};
        for (int i = 0; i < 255; i++)
            if (z.real() * z.real() + z.imag() * z.imag() >= 4)
                return i;
            else
                z = z * z + c;
        return 0;

    }

public:
    explicit Tile(int s);

    Tile() = default;

    Tile(Tile const&) = delete;

    Tile(Tile&&) = delete;

    void operator=(Tile const&) = delete;

    void operator=(Tile&&) = delete;


    void revoke(updateStatus st = updateStatus::STOPPED);

    int getPrior() const noexcept;

    bool isLast() const noexcept;

    void set(Complex cor, Complex diagg) noexcept;

    void add_layer(int val) noexcept {
        std::unique_lock lck(mut);
        layers.emplace_back(val, val, QImage::Format::Format_RGB888);
    }

    void delete_layer() noexcept {
        std::unique_lock lck(mut);
        if (!layers.empty())
            layers.erase(layers.begin());
    }

    int get_cur_size() const {
        std::unique_lock lck(mut);
        return pow(2, currentLayer + 1);
    }

    void set_target_layer(int v) {
        std::unique_lock lck(mut);
        targetLayer += v;
        if (targetLayer > layers.size())
            targetLayer = (int) layers.size() - 1;
        curY = 0;
    }

    updateStatus update() noexcept;

    ~Tile() = default;
};

class priority_Tile {
public:
    int prior;
    Tile* tile;

    auto operator<(priority_Tile const& r) const {
        return prior < r.prior;
    }

    auto operator>(priority_Tile const& r) const {
        return prior > r.prior;
    }

    auto operator==(priority_Tile const& r) const {
        return prior == r.prior;
    }

    auto operator<=(priority_Tile const& r) const {
        return prior <= r.prior;
    }

    auto operator>=(priority_Tile const& r) const {
        return prior >= r.prior;
    }

    auto operator!=(priority_Tile const& r) const {
        return prior != r.prior;
    }
};
