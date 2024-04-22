#include "tile_update.h"

static std::uint8_t mand(long double c_real, long double c_imag) noexcept {
    long double z_real = 0;
    long double z_imag = 0;
    for (int i = 0; i < 255; i++) {
        auto z_real_2 = z_real * z_real;
        auto z_imag_2 = z_imag * z_imag;
        auto z_real_imag = z_real * z_imag;

        if (z_real_2 + z_imag_2 >= 4) {
            return i;
        }

        z_real = z_real_2 - z_imag_2 + c_real;
        z_imag = z_real_imag * 2 + c_imag;
    }
    return 0;
}

void do_update(std::uint8_t* data_ptr, int h, int w, int y,
               int bytesPerLine,
               const std::complex<long double>& diaganal,
               const std::complex<long double>& cor) {
    for (; y < h; y++) {
        std::uint8_t* data = data_ptr + y * bytesPerLine;
        auto yy = (double)y / h * diaganal.imag() + cor.imag();
        for (int x = 0; x < w; x++) {
            auto xx = (double)x / w * diaganal.real() + cor.real();
            std::uint8_t val = mand(xx, yy);
            data[x * 3 + 0] = 0;
            data[x * 3 + 1] = val;
            data[x * 3 + 2] = val;
            //uncomment to drow grid
            //            if (y == 0 || y == h - 1) {
            //                data[x * 3 + 0] = 255;
            //                data[x * 3 + 1] = 0;
            //                data[x * 3 + 2] = 0;
            //            }
        }
        //            data[(w - 1) * 3 + 0] = 255;
        //            data[(w - 1) * 3 + 1] = 0;
        //            data[(w - 1) * 3 + 2] = 0;
        //            data[0] = 255;
        //            data[1] = 0;
        //            data[2] = 0;
    }
}
