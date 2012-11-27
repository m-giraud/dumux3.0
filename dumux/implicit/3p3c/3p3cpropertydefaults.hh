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
 * \ingroup ThreePThreeCModel
 */
/*!
 * \file
 *
 * \brief Defines default values for most properties required by the
 *        3p3c box model.
 */
#ifndef DUMUX_3P3C_PROPERTY_DEFAULTS_HH
#define DUMUX_3P3C_PROPERTY_DEFAULTS_HH

#include "3p3cindices.hh"

#include "3p3cmodel.hh"
#include "3p3cindices.hh"
#include "3p3cfluxvariables.hh"
#include "3p3cvolumevariables.hh"
#include "3p3cproperties.hh"
#include "3p3cnewtoncontroller.hh"
// #include "3p3cboundaryvariables.hh"

#include <dumux/implicit/box/boxdarcyfluxvariables.hh>
#include <dumux/material/spatialparams/boxspatialparams.hh>

namespace Dumux
{

namespace Properties {
//////////////////////////////////////////////////////////////////
// Property values
//////////////////////////////////////////////////////////////////

/*!
 * \brief Set the property for the number of components.
 *
 * We just forward the number from the fluid system and use an static
 * assert to make sure it is 2.
 */
SET_PROP(BoxThreePThreeC, NumComponents)
{
 private:
    typedef typename GET_PROP_TYPE(TypeTag, FluidSystem) FluidSystem;

 public:
    static const int value = FluidSystem::numComponents;

    static_assert(value == 3,
                  "Only fluid systems with 3 components are supported by the 3p3c model!");
};

/*!
 * \brief Set the property for the number of fluid phases.
 *
 * We just forward the number from the fluid system and use an static
 * assert to make sure it is 2.
 */
SET_PROP(BoxThreePThreeC, NumPhases)
{
 private:
    typedef typename GET_PROP_TYPE(TypeTag, FluidSystem) FluidSystem;

 public:
    static const int value = FluidSystem::numPhases;
    static_assert(value == 3,
                  "Only fluid systems with 3 phases are supported by the 3p3c model!");
};

SET_INT_PROP(BoxThreePThreeC, NumEq, 3); //!< set the number of equations to 2

/*!
 * \brief Set the property for the material parameters by extracting
 *        it from the material law.
 */
SET_TYPE_PROP(BoxThreePThreeC, MaterialLawParams, typename GET_PROP_TYPE(TypeTag, MaterialLaw)::Params);

//! The local residual function of the conservation equations
SET_TYPE_PROP(BoxThreePThreeC, LocalResidual, ThreePThreeCLocalResidual<TypeTag>);

//! Use the 3p3c specific newton controller for the 3p3c model
SET_TYPE_PROP(BoxThreePThreeC, NewtonController, ThreePThreeCNewtonController<TypeTag>);

//! the Model property
SET_TYPE_PROP(BoxThreePThreeC, Model, ThreePThreeCModel<TypeTag>);

//! the VolumeVariables property
SET_TYPE_PROP(BoxThreePThreeC, VolumeVariables, ThreePThreeCVolumeVariables<TypeTag>);

//! the FluxVariables property
SET_TYPE_PROP(BoxThreePThreeC, FluxVariables, ThreePThreeCFluxVariables<TypeTag>);

//! define the base flux variables to realize Darcy flow
SET_TYPE_PROP(BoxThreePThreeC, BaseFluxVariables, BoxDarcyFluxVariables<TypeTag>);

//! the upwind factor for the mobility.
SET_SCALAR_PROP(BoxThreePThreeC, ImplicitMassUpwindWeight, 1.0);

//! set default mobility upwind weight to 1.0, i.e. fully upwind
SET_SCALAR_PROP(BoxThreePThreeC, ImplicitMobilityUpwindWeight, 1.0);

//! Determines whether a constraint solver should be used explicitly
SET_BOOL_PROP(BoxThreePThreeC, UseConstraintSolver, false);

//! The indices required by the isothermal 3p3c model
SET_TYPE_PROP(BoxThreePThreeC, Indices, ThreePThreeCIndices<TypeTag, /*PVOffset=*/0>);

//! The spatial parameters to be employed. 
//! Use BoxSpatialParams by default.
SET_TYPE_PROP(BoxThreePThreeC, SpatialParams, BoxSpatialParams<TypeTag>);

// enable gravity by default
SET_BOOL_PROP(BoxThreePThreeC, ProblemEnableGravity, true);

//! default value for the forchheimer coefficient
// Source: Ward, J.C. 1964 Turbulent flow in porous media. ASCE J. Hydraul. Div 90.
//        Actually the Forchheimer coefficient is also a function of the dimensions of the
//        porous medium. Taking it as a constant is only a first approximation
//        (Nield, Bejan, Convection in porous media, 2006, p. 10)
SET_SCALAR_PROP(BoxModel, SpatialParamsForchCoeff, 0.55);

}

}

#endif
