/// \file CIEDE2000.hpp

#ifndef COLORUTIL_CIEDE2000_HPP
#define COLORUTIL_CIEDE2000_HPP

#include <cmath>
#include <color-util/type.hpp>

namespace colorutil
{
    /// \brief  Calculate the perceptual color difference based on CIEDE2000.
    /// \param  color_1  The first color. This should be expressed in CIELAB color space.
    /// \param  color_2  The second color. This should be expressed in CIELAB color space.
    /// \return The color difference of the two colors.
    extern inline double calculate_CIEDE2000(const Lab& color_1, const Lab& color_2);

    /////////////////////////////////////////////////////////////////////////////////

    namespace
    {
        constexpr double epsilon = 1e-10;

        inline double my_atan(double y, double x)
        {
            const double value = std::atan2(y, x) * 180.0 / M_PI;
            return (value < 0.0) ? value + 360.0 : value;
        }

        inline double my_sin(double x) { return std::sin(x * M_PI / 180.0); }

        inline double my_cos(double x) { return std::cos(x * M_PI / 180.0); }

        inline double get_h_prime(double a_prime, double b)
        {
            const bool a_prime_and_b_are_zeros = (std::abs(a_prime) < epsilon) && (std::abs(b) < epsilon);
            return a_prime_and_b_are_zeros ? 0.0 : my_atan(b, a_prime);
        }

        inline double get_delta_h_prime(double C_1_prime, double C_2_prime, double h_1_prime, double h_2_prime)
        {
            if (C_1_prime * C_2_prime < epsilon)
            {
                return 0.0;
            }

            const double diff = h_2_prime - h_1_prime;

            if (std::abs(diff) <= 180.0)
            {
                return diff;
            }
            else if (diff > 180.0)
            {
                return diff - 360.0;
            }
            else
            {
                return diff + 360.0;
            }
        }

        inline double get_h_prime_bar(double C_1_prime, double C_2_prime, double h_1_prime, double h_2_prime)
        {
            if (C_1_prime * C_2_prime < epsilon)
            {
                return h_1_prime + h_2_prime;
            }

            const double dist = std::abs(h_1_prime - h_2_prime);
            const double sum  = h_1_prime + h_2_prime;

            if (dist <= 180.0)
            {
                return 0.5 * sum;
            }
            else if (sum < 360.0)
            {
                return 0.5 * (sum + 360.0);
            }
            else
            {
                return 0.5 * (sum - 360.0);
            }
        }
    } // namespace

    inline double calculate_CIEDE2000(const Lab& color_1, const Lab& color_2)
    {
        const double& L_1 = color_1(0);
        const double& a_1 = color_1(1);
        const double& b_1 = color_1(2);
        const double& L_2 = color_2(0);
        const double& a_2 = color_2(1);
        const double& b_2 = color_2(2);

        // Step 1

        const double C_1_ab   = std::sqrt(a_1 * a_1 + b_1 * b_1);
        const double C_2_ab   = std::sqrt(a_2 * a_2 + b_2 * b_2);
        const double C_ab_bar = 0.5 * (C_1_ab + C_2_ab);
        const double G =
            0.5 * (1.0 - std::sqrt(std::pow(C_ab_bar, 7.0) / (std::pow(C_ab_bar, 7.0) + std::pow(25.0, 7.0))));
        const double a_1_prime = (1.0 + G) * a_1;
        const double a_2_prime = (1.0 + G) * a_2;
        const double C_1_prime = std::sqrt(a_1_prime * a_1_prime + b_1 * b_1);
        const double C_2_prime = std::sqrt(a_2_prime * a_2_prime + b_2 * b_2);
        const double h_1_prime = get_h_prime(a_1_prime, b_1);
        const double h_2_prime = get_h_prime(a_2_prime, b_2);

        // Step 2

        const double delta_L_prime = L_2 - L_1;
        const double delta_C_prime = C_2_prime - C_1_prime;
        const double delta_h_prime = get_delta_h_prime(C_1_prime, C_2_prime, h_1_prime, h_2_prime);
        const double delta_H_prime = 2.0 * std::sqrt(C_1_prime * C_2_prime) * my_sin(0.5 * delta_h_prime);

        // Step 3

        const double L_prime_bar = 0.5 * (L_1 + L_2);
        const double C_prime_bar = 0.5 * (C_1_prime + C_2_prime);
        const double h_prime_bar = get_h_prime_bar(C_1_prime, C_2_prime, h_1_prime, h_2_prime);

        const double T = 1.0 - 0.17 * my_cos(h_prime_bar - 30.0) + 0.24 * my_cos(2.0 * h_prime_bar) +
                         0.32 * my_cos(3.0 * h_prime_bar + 6.0) - 0.20 * my_cos(4.0 * h_prime_bar - 63.0);

        const double delta_theta = 30.0 * std::exp(-((h_prime_bar - 275) / 25.0) * ((h_prime_bar - 275) / 25.0));

        const double R_C =
            2.0 * std::sqrt(std::pow(C_prime_bar, 7.0) / (std::pow(C_prime_bar, 7.0) + std::pow(25.0, 7.0)));
        const double S_L = 1.0 + (0.015 * (L_prime_bar - 50.0) * (L_prime_bar - 50.0)) /
                                     std::sqrt(20.0 + (L_prime_bar - 50.0) * (L_prime_bar - 50.0));
        const double S_C = 1.0 + 0.045 * C_prime_bar;
        const double S_H = 1.0 + 0.015 * C_prime_bar * T;
        const double R_T = -my_sin(2.0 * delta_theta) * R_C;

        constexpr double k_L = 1.0;
        constexpr double k_C = 1.0;
        constexpr double k_H = 1.0;

        const double delta_L = delta_L_prime / (k_L * S_L);
        const double delta_C = delta_C_prime / (k_C * S_C);
        const double delta_H = delta_H_prime / (k_H * S_H);

        const double delta_E_squared =
            delta_L * delta_L + delta_C * delta_C + delta_H * delta_H + R_T * delta_C * delta_H;

        return std::sqrt(delta_E_squared);
    }
} // namespace colorutil

#endif // COLORUTIL_CIEDE2000_HPP
