// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*****************************************************************************
 *   Copyright (C) 2009 by Andreas Lauser
 *   Institute for Modelling Hydraulic and Environmental Systems             *
 *   University of Stuttgart, Germany                                        *
 *   email: <givenname>.<name>@iws.uni-stuttgart.de                          *
 *                                                                           *
 *   This program is free software: you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation, either version 2 of the License, or       *
 *   (at your option) any later version.                                     *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *   GNU General Public License for more details.                            *
 *                                                                           *
 *   You should have received a copy of the GNU General Public License       *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 *****************************************************************************/
/*!
 * \file
 * \ingroup Components
 * \brief  A simple implementation of a DNAPL.
 */
#ifndef DUMUX_SIMPLE_DNAPL_HH
#define DUMUX_SIMPLE_DNAPL_HH

#warning use material/components/dnapl.hh instead

#include "dnapl.hh"

namespace Dumux
{
/*!
 * \ingroup Components
 * \brief A much simple component for an exemplary dense NAPL (TCE).
 *
 * \tparam Scalar The type used for scalar values
 */
template <class Scalar>
class SimpleDNAPL : public DNAPL<Scalar>
{

public:
    DUNE_DEPRECATED_MSG("use DNAPL instead")
    SimpleDNAPL(){}
};

} // end namepace

#endif
