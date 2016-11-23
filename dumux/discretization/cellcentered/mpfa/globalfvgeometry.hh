// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*****************************************************************************
 *   See the file COPYING for full copying permissions.                      *
 *                                                                           *
 *   This program is free software: you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation, either version 2 of the License, or       *
 *   (at your option) any later version.                                     *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the            *
 *   GNU General Public License for more details.                            *
 *                                                                           *
 *   You should have received a copy of the GNU General Public License       *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 *****************************************************************************/
/*!
 * \file
 * \brief Base class for the finite volume geometry vector for box models
 *        This builds up the sub control volumes and sub control volume faces
 *        for each element.
 */
#ifndef DUMUX_DISCRETIZATION_CC_MPFA_GLOBALFVGEOMETRY_HH
#define DUMUX_DISCRETIZATION_CC_MPFA_GLOBALFVGEOMETRY_HH

#include <dumux/discretization/cellcentered/mpfa/globalfvgeometrybase.hh>
#include "methods.hh"

namespace Dumux
{
//! Forward declaration of the method-specific specialization
//! By default, the methods simply inherit from the base class
//! Methods that use a different implementation have to provide this below
template<class TypeTag, MpfaMethods method, bool EnableFVElementGeometryCache>
class CCMpfaGlobalFVGeometryImplementation : public CCMpfaGlobalFVGeometryBase<TypeTag, EnableFVElementGeometryCache>
{
    using ParentType = CCMpfaGlobalFVGeometryBase<TypeTag, EnableFVElementGeometryCache>;
    using GridView = typename GET_PROP_TYPE(TypeTag, GridView);

public:
    CCMpfaGlobalFVGeometryImplementation(const GridView& gridView) : ParentType(gridView) {}
};

/*!
 * \ingroup ImplicitModel
 * \brief Base class for the finite volume geometry vector for mpfa models
 *        This builds up the sub control volumes and sub control volume faces
 *        for each element.
 */
template<class TypeTag, bool EnableFVElementGeometryCache>
using CCMpfaGlobalFVGeometry = CCMpfaGlobalFVGeometryImplementation<TypeTag, GET_PROP_VALUE(TypeTag, MpfaMethod), EnableFVElementGeometryCache>;

} // end namespace

// further specializations of this class for the available methods have to be included here

#endif
