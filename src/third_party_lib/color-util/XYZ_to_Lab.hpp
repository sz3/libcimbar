/// \file XYZ_to_Lab.hpp

#ifndef COLORUTIL_XYZ_TO_LAB_HPP
#define COLORUTIL_XYZ_TO_LAB_HPP

#include <cmath>
#include <color-util/type.hpp>

namespace colorutil
{
    /// \brief Convert a CIEXYZ color into a CIELAB color under Illuminant D65.
    /// \param xyz_color A color represented in CIEXYZ (D65)
    /// \return A color represented in CIELAB (D65)
    inline Lab convert_XYZ_to_Lab(const XYZ& xyz_color)
    {
        // The reference white point under Illuminant D65
        constexpr double ref_xyz[3]     = {95.047, 100.000, 108.883};
        constexpr double inv_ref_xyz[3] = {1.0 / ref_xyz[0], 1.0 / ref_xyz[1], 1.0 / ref_xyz[2]};

        auto f = [](double t) {
            constexpr double delta  = 6.0 / 29.0;
            constexpr double delta3 = delta * delta * delta;

            if (t > delta3)
            {
                return std::pow(t, 1.0 / 3.0);
            }
            else
            {
                return t / (3.0 * delta * delta) + 4.0 / 29.0;
            }
        };

        const Eigen::Vector3d normalized_xyz = xyz_color.cwiseProduct(Eigen::Map<const Eigen::Vector3d>(inv_ref_xyz));

        const double f_x = f(normalized_xyz(0));
        const double f_y = f(normalized_xyz(1));
        const double f_z = f(normalized_xyz(2));

        const double L = 116.0 * f_y - 16.0;
        const double a = 500.0 * (f_x - f_y);
        const double b = 200.0 * (f_y - f_z);

        return Lab(L, a, b);
    }
} // namespace colorutil

#endif // COLORUTIL_XYZ_TO_LAB_HPP
