/// \file RGB_to_HSL.hpp

#ifndef COLORUTIL_RGB_TO_HSL_HPP
#define COLORUTIL_RGB_TO_HSL_HPP

#include <cmath>
#include <color-util/type.hpp>

namespace colorutil
{
    /// \param rgb_color A color represented in RGB
    /// \return A color represented in HSL
    inline HSL convert_RGB_to_HSL(const RGB& rgb_color)
    {
        int max_coeff_index;

        const double M = rgb_color.maxCoeff(&max_coeff_index);
        const double m = rgb_color.minCoeff();
        const double C = M - m;

        const double L = 0.5 * (M + m);
        const double S = L > 0.99999 ? 0.0 : C / (1.0 - std::abs(2.0 * L - 1.0));
        const double H = [&]() {
            double H_prime;

            if (C < 0.00001)
            {
                H_prime = 0.0;
            }
            else if (max_coeff_index == 0)
            {
                H_prime = ((rgb_color(1) - rgb_color(2)) / C);
            }
            else if (max_coeff_index == 1)
            {
                H_prime = ((rgb_color(2) - rgb_color(0)) / C) + 2.0;
            }
            else if (max_coeff_index == 2)
            {
                H_prime = ((rgb_color(0) - rgb_color(1)) / C) + 4.0;
            }
            else
            {
                assert(false);
            }

            return H_prime > 0.0 ? H_prime / 6.0 : H_prime / 6.0 + 1.0;
        }();

        return HSL(H, S, L);
    }
} // namespace colorutil

#endif // COLORUTIL_RGB_TO_HSL_HPP
