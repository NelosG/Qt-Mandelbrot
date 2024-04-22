#include "tile_update.h"

__device__ std::uint8_t mand(double c_real,  double c_imag) noexcept {
    double z_real = 0;
    double z_imag = 0;
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

__global__ void do_update_cuda(std::uint8_t* data_ptr, int h, int w, int y,
                               int bytesPerLine,
                               double diaganal_real,
                               double diaganal_imag,
                               double cor_real,
                               double cor_imag
) {
    auto curY = y + threadIdx.x;

    std::uint8_t* data = data_ptr + curY * bytesPerLine;
    auto yy = (double)curY / h * diaganal_imag + cor_imag;
    for (int x = 0; x < w; x++) {
        auto xx = (double)x / w * diaganal_real + cor_real;
        std::uint8_t val = mand(xx, yy);
        data[x * 3 + 0] = 0;
        data[x * 3 + 1] = val;
        data[x * 3 + 2] = val;
    }
}

void do_update(std::uint8_t* data_ptr, int h, int w, int y,
               int bytesPerLine,
               const std::complex<long double>& diaganal,
               const std::complex<long double>& cor) {
    auto bytes = h * bytesPerLine;
    std::uint8_t* p;
    // auto* p = (std::uint8_t*)malloc(sizeof(std::uint8_t) * bytes);
    cudaMalloc((void**)&p, sizeof(std::uint8_t) * bytes);

    do_update_cuda<<<1, h - y>>>(p,
                                 h, w, y,
                                 bytesPerLine,
                                 diaganal.real(),
                                 diaganal.imag(),
                                 cor.real(),
                                 cor.imag()
    );
    cudaMemcpy(data_ptr, p, sizeof(std::uint8_t) * bytes, cudaMemcpyDeviceToHost);
    cudaFree(p);
}
