/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2022 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

#ifndef NATRON_ENGINE_TEXTURERECTSERIALIZATION_H
#define NATRON_ENGINE_TEXTURERECTSERIALIZATION_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
GCC_DIAG_OFF(unused-parameter)
// /opt/local/include/boost/serialization/smart_cast.hpp:254:25: warning: unused parameter 'u' [-Wunused-parameter]
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/version.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
GCC_DIAG_ON(unused-parameter)
#endif
#include "Engine/TextureRect.h"
#include "Engine/EngineFwd.h"


// Note: these classes are used for cache serialization and do not have to maintain backward compatibility
#define TEXTURE_RECT_SERIALIZATION_INTRODUCES_PAR 2
#define TEXTURE_RECT_VERSION TEXTURE_RECT_SERIALIZATION_INTRODUCES_PAR

namespace boost {
namespace serialization {
template<class Archive>
void
serialize(Archive & ar,
          NATRON_NAMESPACE::TextureRect &t,
          const unsigned int /*version*/)
{
    ar &
    ::boost::serialization::make_nvp("x1", t.x1) &
    ::boost::serialization::make_nvp("x2", t.x2) &
    ::boost::serialization::make_nvp("y1", t.y1) &
    ::boost::serialization::make_nvp("y2", t.y2) &
    ::boost::serialization::make_nvp("po2", t.closestPo2) &
    ::boost::serialization::make_nvp("par", t.par);
}
}
}

BOOST_CLASS_VERSION(NATRON_NAMESPACE::TextureRect, TEXTURE_RECT_VERSION);

#endif // NATRON_ENGINE_TEXTURERECTSERIALIZATION_H
