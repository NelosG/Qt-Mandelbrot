#pragma once

#include <chrono>

class Timer {
    using clock_t = std::chrono::high_resolution_clock;
    using second_t = std::chrono::duration<double>;

    std::chrono::time_point<clock_t> m_beg;

    public:
        Timer() : m_beg(clock_t::now()) {}

        void reset() {
            m_beg = clock_t::now();
        }

        [[nodiscard]] double elapsed() const {
            return std::chrono::duration_cast<second_t>(clock_t::now() - m_beg).count();
        }
};
