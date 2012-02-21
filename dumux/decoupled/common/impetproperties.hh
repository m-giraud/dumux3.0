// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*****************************************************************************
 *   Copyright (C) 2009 by Markus Wolff                                      *
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
#ifndef DUMUX_IMPET_PROPERTIES_HH
#define DUMUX_IMPET_PROPERTIES_HH

#include <dumux/decoupled/common/decoupledproperties.hh>
#include <dumux/decoupled/common/pressureproperties.hh>
#include <dumux/decoupled/common/transportproperties.hh>

/*!
 * \ingroup IMPET
 * \ingroup IMPETProperties
 */
/*!
 * \file
 * \brief Base file for properties related to sequential IMPET algorithms
 */
namespace Dumux
{

template<class TypeTag>
class IMPET;

namespace Properties
{
/*!
 *
 * \brief General properties for sequential IMPET algorithms
 *
 * This class holds properties necessary for the sequential IMPET solution.
 */

//////////////////////////////////////////////////////////////////
// Type tags tags
//////////////////////////////////////////////////////////////////

//! The type tag for models based on the diffusion-scheme
NEW_TYPE_TAG(IMPET, INHERITS_FROM(DecoupledModel));

//////////////////////////////////////////////////////////////////
// Property tags
//////////////////////////////////////////////////////////////////

NEW_PROP_TAG(CFLFactor);         //!< Scalar factor for additional scaling of the time step
NEW_PROP_TAG(IterationFlag); //!< Flag to switch the iteration type of the IMPET scheme
NEW_PROP_TAG(IterationNumber); //!< Number of iterations if IMPET iterations are enabled by the IterationFlags
NEW_PROP_TAG(MaximumDefect); //!< Maximum Defect if IMPET iterations are enabled by the IterationFlags
NEW_PROP_TAG(RelaxationFactor); //!< Used for IMPET iterations

//forward declaration!
NEW_PROP_TAG( Model );//! The model of the specific problem
}
}

#include <dumux/decoupled/common/impet.hh>

namespace Dumux
{
namespace Properties
{
//set impet model
SET_TYPE_PROP(IMPET, Model, IMPET<TypeTag>);

//Set defaults
SET_SCALAR_PROP(IMPET, CFLFactor, 1.0);
SET_INT_PROP(IMPET, IterationFlag, 0); //!< 0 = no iterations, 1 = iterate IterationNumber iterations, 2 = iterate until converged or IterationNumber is reached
SET_INT_PROP(IMPET, IterationNumber, 2);
SET_SCALAR_PROP(IMPET, MaximumDefect, 1e-5);
SET_SCALAR_PROP(IMPET, RelaxationFactor, 1.0);//!< 1 = new solution is new solution, 0 = old solution is new solution
}
}

#endif
