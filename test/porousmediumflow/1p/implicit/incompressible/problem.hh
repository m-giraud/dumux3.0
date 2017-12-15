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
 * \brief The properties for the incompressible test
 */
#ifndef DUMUX_INCOMPRESSIBLE_ONEP_TEST_PROBLEM_HH
#define DUMUX_INCOMPRESSIBLE_ONEP_TEST_PROBLEM_HH

#include <dumux/discretization/cellcentered/tpfa/properties.hh>
#include <dumux/discretization/cellcentered/mpfa/properties.hh>
#include <dumux/discretization/box/properties.hh>

#include <dumux/porousmediumflow/problem.hh>
#include <dumux/porousmediumflow/1p/implicit/model.hh>
#include <dumux/porousmediumflow/1p/implicit/incompressiblelocalresidual.hh>

#include <dumux/material/components/simpleh2o.hh>
#include <dumux/material/fluidsystems/liquidphase.hh>
#include <dumux/material/fluidsystems/1p.hh>

#include "spatialparams.hh"

namespace Dumux
{
// forward declarations
template<class TypeTag> class OnePTestProblem;

namespace Properties
{
// create the type tag nodes
NEW_TYPE_TAG(OnePIncompressible, INHERITS_FROM(OneP));
NEW_TYPE_TAG(OnePIncompressibleTpfa, INHERITS_FROM(CCTpfaModel, OnePIncompressible));
NEW_TYPE_TAG(OnePIncompressibleMpfa, INHERITS_FROM(CCMpfaModel, OnePIncompressible));
NEW_TYPE_TAG(OnePIncompressibleBox, INHERITS_FROM(BoxModel, OnePIncompressible));

// Set the grid type
SET_TYPE_PROP(OnePIncompressible, Grid, Dune::YaspGrid<2>);

// Set the problem type
SET_TYPE_PROP(OnePIncompressible, Problem, OnePTestProblem<TypeTag>);

// set the spatial params
SET_TYPE_PROP(OnePIncompressible, SpatialParams, OnePTestSpatialParams<TypeTag>);

// use the incompressible local residual (provides analytic jacobian)
SET_TYPE_PROP(OnePIncompressible, LocalResidual, OnePIncompressibleLocalResidual<TypeTag>);

// the fluid system
SET_PROP(OnePIncompressible, FluidSystem)
{
    using Scalar = typename GET_PROP_TYPE(TypeTag, Scalar);
    using type = FluidSystems::OneP<Scalar, FluidSystems::LiquidPhase<Scalar, SimpleH2O<Scalar> > >;
};

// Enable caching
SET_BOOL_PROP(OnePIncompressible, EnableGridVolumeVariablesCache, false);
SET_BOOL_PROP(OnePIncompressible, EnableGridFluxVariablesCache, false);
SET_BOOL_PROP(OnePIncompressible, EnableFVGridGeometryCache, false);

} // end namespace Properties

template<class TypeTag>
class OnePTestProblem : public PorousMediumFlowProblem<TypeTag>
{
    using ParentType = PorousMediumFlowProblem<TypeTag>;
    using GridView = typename GET_PROP_TYPE(TypeTag, GridView);
    using Element = typename GridView::template Codim<0>::Entity;
    using Scalar = typename GET_PROP_TYPE(TypeTag, Scalar);
    using PrimaryVariables = typename GET_PROP_TYPE(TypeTag, PrimaryVariables);
    using ResidualVector = typename GET_PROP_TYPE(TypeTag, NumEqVector);
    using FVGridGeometry = typename GET_PROP_TYPE(TypeTag, FVGridGeometry);
    using FVElementGeometry = typename GET_PROP_TYPE(TypeTag, FVElementGeometry);
    using ElementVolumeVariables = typename GET_PROP_TYPE(TypeTag, ElementVolumeVariables);
    using BoundaryTypes = typename GET_PROP_TYPE(TypeTag, BoundaryTypes);
    using SubControlVolume = typename GET_PROP_TYPE(TypeTag, SubControlVolume);
    using ElementSolutionVector = typename GET_PROP_TYPE(TypeTag, ElementSolutionVector);
    using PointSource = typename GET_PROP_TYPE(TypeTag, PointSource);
    static constexpr int dimWorld = GridView::dimensionworld;
    using GlobalPosition = Dune::FieldVector<Scalar, dimWorld>;

public:
    OnePTestProblem(std::shared_ptr<const FVGridGeometry> fvGridGeometry)
    : ParentType(fvGridGeometry) {}

    /*!
     * \brief Specifies which kind of boundary condition should be
     *        used for which equation on a given boundary control volume.
     *
     * \param values The boundary types for the conservation equations
     * \param globalPos The position of the center of the finite volume
     */
    BoundaryTypes boundaryTypesAtPos(const GlobalPosition &globalPos) const
    {
        BoundaryTypes values;

        Scalar eps = 1.0e-6;
        if (globalPos[dimWorld-1] < eps || globalPos[dimWorld-1] > this->fvGridGeometry().bBoxMax()[dimWorld-1] - eps)
            values.setAllDirichlet();
        else
            values.setAllNeumann();

        return values;
    }

    /*!
     * \brief Evaluate the boundary conditions for a dirichlet
     *        control volume.
     *
     * \param values The dirichlet values for the primary variables
     * \param globalPos The center of the finite volume which ought to be set.
     *
     * For this method, the \a values parameter stores primary variables.
     */
    PrimaryVariables dirichletAtPos(const GlobalPosition &globalPos) const
    {
        PrimaryVariables values(0);
        values[0] = 1.0e+5*(2.0 - globalPos[dimWorld-1]);
        return values;
    }

    /*!
     * \brief Returns the temperature \f$\mathrm{[K]}\f$ for an isothermal problem.
     *
     * This is not specific to the discretization. By default it just
     * throws an exception so it must be overloaded by the problem if
     * no energy equation is used.
     */
    Scalar temperature() const
    {
        return 283.15; // 10°C
    }
};

} // end namespace Dumux

#endif
