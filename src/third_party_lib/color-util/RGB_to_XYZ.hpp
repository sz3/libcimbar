/// \file RGB_to_XYZ.hpp

#ifndef COLORUTIL_RGB_TO_XYZ_HPP
#define COLORUTIL_RGB_TO_XYZ_HPP

#include <cmath>
#include <color-util/type.hpp>

namespace colorutil
{
    /// \brief Perform the inverse gamma companding for a sRGB color
    /// \details http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
    inline RGB perform_inverse_sRGB_companding(const RGB& srgb_color)
    {
        RGB linear_srgb_color;
        for (int i : {0, 1, 2})
        {
            linear_srgb_color(i) =
                (srgb_color(i) <= 0.04045) ? srgb_color(i) / 12.92 : std::pow((srgb_color(i) + 0.055) / 1.055, 2.4);
        }
        return linear_srgb_color;
    }

    /// \param rgb_color A color represented in sRGB (D65)
    /// \return A color represented in CIEXYZ (D65)
    inline XYZ convert_RGB_to_XYZ(const RGB& srgb_color)
    {
        // Inverse companding for sRGB
        const RGB linear_srgb = perform_inverse_sRGB_companding(srgb_color);

        // Retrieved from http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
        // sRGB (D65)
        constexpr double M_data[3 * 3] = {
            0.4124564, 0.3575761, 0.1804375, 0.2126729, 0.7151522, 0.0721750, 0.0193339, 0.1191920, 0.9503041};

        // Note: The use of "auto" avoids unnecessary data copy by lazy evaluation
        const auto M = Eigen::Map<const Eigen::Matrix<double, 3, 3, Eigen::RowMajor>>(M_data);

        return 100.0 * M * linear_srgb;
    }
} // namespace colorutil

#endif // COLORUTIL_RGB_TO_XYZ_HPP
