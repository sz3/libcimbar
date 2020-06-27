/// \file type.hpp

#ifndef COLORUTIL_TYPE_HPP
#define COLORUTIL_TYPE_HPP

#include <Eigen/Core>

namespace colorutil
{
    /// \brief RGB
    ///
    /// \details Each value is defined in [0, 1].
    using RGB = Eigen::Vector3d;

    /// \brief HSL
    ///
    /// \details Each value is defined in [0, 1].
    using HSL = Eigen::Vector3d;

    /// \brief CIEXYZ
    using XYZ = Eigen::Vector3d;

    /// \brief CIELAB
    using Lab = Eigen::Vector3d;
} // namespace colorutil

#endif // COLORUTIL_TYPE_HPP
