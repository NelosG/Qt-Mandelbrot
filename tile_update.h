#pragma once
#include <complex>

void do_update(std::uint8_t* data_ptr, int h, int w, int y,
               int bytesPerLine,
               const std::complex<long double>& diaganal,
               const std::complex<long double>& cor);

