/// \file HSL_to_RGB.hpp

#ifndef COLORUTIL_HSL_TO_RGB_HPP
#define COLORUTIL_HSL_TO_RGB_HPP

#include <cmath>
#include <color-util/type.hpp>

namespace colorutil
{
    /// \param hsl_color A color represented in HSL
    /// \return A color represented in RGB
    inline RGB convert_HSL_to_RGB(const HSL& hsl_color)
    {
        const double H = hsl_color(0);
        const double S = hsl_color(1);
        const double L = hsl_color(2);

        const double C = (1.0 - std::abs(2.0 * L - 1.0)) * S;
        const double m = L - 0.5 * C;

        const double H_prime       = H * 6.0;
        const double H_prime_mod_2 = [&]() {
            if (H_prime < 2.0)
            {
                return H_prime;
            }
            else if (H_prime < 4.0)
            {
                return H_prime - 2.0;
            }
            else if (H_prime <= 6.0)
            {
                return H_prime - 4.0;
            }
            else
            {
                assert(false);
            }
        }();

        const double X = C * (1.0 - std::abs(H_prime_mod_2 - 1.0));

        const RGB rgb_color_base = [&]() {
            if (H_prime < 1.0)
            {
                return RGB(C, X, 0.0);
            }
            else if (H_prime < 2.0)
            {
                return RGB(X, C, 0.0);
            }
            else if (H_prime < 3.0)
            {
                return RGB(0.0, C, X);
            }
            else if (H_prime < 4.0)
            {
                return RGB(0.0, X, C);
            }
            else if (H_prime < 5.0)
            {
                return RGB(X, 0.0, C);
            }
            else if (H_prime <= 6.0)
            {
                return RGB(C, 0.0, X);
            }
            else
            {
                assert(false);
            }
        }();

        return rgb_color_base + RGB(m, m, m);
    }
} // namespace colorutil

#endif // COLORUTIL_HSL_TO_RGB_HPP
