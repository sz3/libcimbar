/// \file CIE76.hpp

#ifndef COLORUTIL_CIE76_HPP
#define COLORUTIL_CIE76_HPP

#include <cmath>
#include <color-util/type.hpp>

namespace colorutil
{
    /// \brief  Calculate the perceptual color difference based on CIE76.
    /// \param  color_1  The first color. This should be expressed in CIELAB color space.
    /// \param  color_2  The second color. This should be expressed in CIELAB color space.
    /// \return The color difference of the two colors.
    inline double calculate_CIE76(const Lab& color_1, const Lab& color_2) { return (color_1 - color_2).norm(); }
} // namespace colorutil

#endif // COLORUTIL_CIE76_HPP
