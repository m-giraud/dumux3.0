// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * \brief test problem for diffusion models from the FVCA6 benchmark.
 */
#ifndef DUMUX_TEST_DIFFUSION_3D_PROBLEM_HH
#define DUMUX_TEST_DIFFUSION_3D_PROBLEM_HH

#if HAVE_ALUGRID
#include <dune/grid/alugrid/3d/alugrid.hh>
#endif
#include <dune/grid/yaspgrid.hh>

#include <dumux/material/components/unit.hh>

#include <dumux/decoupled/2p/diffusion/fv/fvpressureproperties2p.hh>
#include <dumux/decoupled/2p/diffusion/fvmpfa/lmethod/fvmpfal3dpressureproperties2p.hh>
#include <dumux/decoupled/2p/diffusion/mimetic/mimeticpressureproperties2p.hh>

#include <dumux/decoupled/2p/diffusion/diffusionproblem2p.hh>
#include <dumux/decoupled/common/fv/fvvelocity.hh>

#include "test_diffusionspatialparams3d.hh"

namespace Dumux
{
/*!
 * \ingroup IMPETtests
 */
template<class TypeTag>
class TestDiffusion3DProblem;

//////////
// Specify the properties
//////////
namespace Properties
{
NEW_TYPE_TAG(DiffusionTestProblem, INHERITS_FROM(DecoupledTwoP, TestDiffusionSpatialParams3d));

// Set the grid type
SET_PROP(DiffusionTestProblem, Grid)
{
#if HAVE_ALUGRID
    typedef Dune::ALUGrid<3, 3, Dune::cube, Dune::nonconforming> type;
#else
    typedef Dune::YaspGrid<3> type;
#endif
};

SET_TYPE_PROP(DiffusionTestProblem, Problem, Dumux::TestDiffusion3DProblem<TypeTag>);

// Set the wetting phase
SET_PROP(DiffusionTestProblem, WettingPhase)
{
private:
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
public:
    typedef Dumux::LiquidPhase<Scalar, Dumux::Unit<Scalar> > type;
};

// Set the non-wetting phase
SET_PROP(DiffusionTestProblem, NonwettingPhase)
{
private:
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
public:
    typedef Dumux::LiquidPhase<Scalar, Dumux::Unit<Scalar> > type;
};

// Enable gravity
SET_BOOL_PROP(DiffusionTestProblem, ProblemEnableGravity, false);

#if HAVE_SUPERLU
SET_TYPE_PROP(DiffusionTestProblem, LinearSolver, Dumux::SuperLUBackend<TypeTag>);
#else
SET_TYPE_PROP(DiffusionTestProblem, LinearSolver, Dumux::ILUnRestartedGMResBackend<TypeTag>);
SET_INT_PROP(DiffusionTestProblem, LinearSolverGMResRestart, 80);
SET_INT_PROP(DiffusionTestProblem, LinearSolverMaxIterations, 1000);
SET_SCALAR_PROP(DiffusionTestProblem, LinearSolverResidualReduction, 1e-8);
SET_SCALAR_PROP(DiffusionTestProblem, LinearSolverPreconditionerIterations, 1);
#endif

// set the types for the 2PFA FV method
NEW_TYPE_TAG(FVTestProblem, INHERITS_FROM(FVPressureTwoP, DiffusionTestProblem));

// set the types for the MPFA-L FV method
NEW_TYPE_TAG(FVMPFAL3DTestProblem, INHERITS_FROM(FvMpfaL3dPressureTwoP, DiffusionTestProblem));
SET_INT_PROP(FVMPFAL3DTestProblem, MPFATransmissibilityCriterion, 1);

// set the types for the mimetic FD method
NEW_TYPE_TAG(MimeticTestProblem, INHERITS_FROM(MimeticPressureTwoP, DiffusionTestProblem));
}

/*!
 * \ingroup DecoupledProblems
 *
 * \brief test problem for diffusion models from the FVCA6 benchmark.
 */
template<class TypeTag>
class TestDiffusion3DProblem: public DiffusionProblem2P<TypeTag>
{
    typedef DiffusionProblem2P<TypeTag> ParentType;
    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;

    typedef typename GET_PROP_TYPE(TypeTag, Indices) Indices;

    typedef typename GET_PROP_TYPE(TypeTag, FluidSystem) FluidSystem;
    typedef typename GET_PROP_TYPE(TypeTag, FluidState) FluidState;

    typedef typename GET_PROP_TYPE(TypeTag, BoundaryTypes) BoundaryTypes;

    enum
    {
        dim = GridView::dimension, dimWorld = GridView::dimensionworld
    };

    enum
    {
        wPhaseIdx = Indices::wPhaseIdx,
        nPhaseIdx = Indices::nPhaseIdx,
        pWIdx = Indices::pressureIdx,
        swIdx = Indices::swIdx,
        pressureEqIdx = Indices::pressureEqIdx,
    };

    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;

    typedef typename GridView::template Codim<0>::Iterator ElementIterator;
    typedef typename GridView::Traits::template Codim<0>::Entity Element;
    typedef typename GridView::Intersection Intersection;
    typedef Dune::FieldVector<Scalar, dimWorld> GlobalPosition;

public:
    typedef typename GET_PROP(TypeTag, SolutionTypes) SolutionTypes;
    typedef typename SolutionTypes::PrimaryVariables PrimaryVariables;
    typedef typename SolutionTypes::ScalarSolution ScalarSolution;

    TestDiffusion3DProblem(const GridView &gridView) :
        ParentType(gridView), velocity_(*this)
    { }

    //!for this specific problem: initialize the saturation and afterwards the model
    void init()
    {
        this->variables().initialize();
        for (int i = 0; i < this->gridView().size(0); i++)
        {
            this->variables().cellData(i).setSaturation(wPhaseIdx, 1.0);
            this->variables().cellData(i).setSaturation(nPhaseIdx, 0.0);
        }
        this->model().initialize();
    }

    void calculateFVVelocity()
    {
        velocity_.calculateVelocity();
//        velocity_.addOutputVtkFields(this->resultWriter());
    }

    //! \copydoc ParentType::addOutputVtkFields()
    void addOutputVtkFields()
    {
        ScalarSolution *exactPressure = this->resultWriter().allocateManagedBuffer(this->gridView().size(0));

        ElementIterator eIt = this->gridView().template begin<0>();
        ElementIterator eItEnd = this->gridView().template end<0>();
        for(;eIt != eItEnd; ++eIt)
        {
            (*exactPressure)[this->elementMapper().map(*eIt)][0] = exact(eIt->geometry().center());
        }

        this->resultWriter().attachCellData(*exactPressure, "exact pressure");

        return;
    }

    /*!
    * \name Problem parameters
    */
    // \{

    bool shouldWriteRestartFile() const
    { return false; }

    /*!
     * \brief Returns the temperature within the domain.
     *
     * This problem assumes a temperature of 10 degrees Celsius.
     */
    Scalar temperatureAtPos(const GlobalPosition& globalPos) const
    {
        return 273.15 + 10; // -> 10°C
    }

    // \}


    //! Returns the reference pressure for evaluation of constitutive relations
    Scalar referencePressureAtPos(const GlobalPosition& globalPos) const
    {
        return 1e5; // -> 10°C
    }

    void sourceAtPos(PrimaryVariables &values,const GlobalPosition& globalPos) const
    {
        values = 0;

        double pi = 4.0*atan(1.0);

        values[wPhaseIdx] = -pi*pi*cos(pi*globalPos[0])*cos(pi*(globalPos[1]+1.0/2.0))*sin(pi*(globalPos[2]+1.0/3.0))+
                            3.0*pi*pi*sin(pi*globalPos[0])*sin(pi*(globalPos[1]+1.0/2.0))*sin(pi*(globalPos[2]+1.0/3.0))-
                            pi*pi*sin(pi*globalPos[0])*cos(pi*(globalPos[1]+1.0/2.0))*cos(pi*(globalPos[2]+1.0/3.0));
    }

    /*!
    * \brief Returns the type of boundary condition.
    *
    *
    * BC for saturation equation can be dirichlet (saturation), neumann (flux), or outflow.
    */
    void boundaryTypesAtPos(BoundaryTypes &bcTypes, const GlobalPosition& globalPos) const
    {
                bcTypes.setAllDirichlet();
    }

    //! set dirichlet condition  (saturation [-])
    void dirichletAtPos(PrimaryVariables &values, const GlobalPosition& globalPos) const
    {
            values[pWIdx] = exact(globalPos);
            values[swIdx] = 1.0;
    }

    //! set neumann condition for phases (flux, [kg/(m^2 s)])
    void neumannAtPos(PrimaryVariables &values, const GlobalPosition& globalPos) const
    {
        values = 0;
    }

    Scalar exact (const GlobalPosition& globalPos) const
    {
        double pi = 4.0*atan(1.0);

        return (1.0+sin(pi*globalPos[0])*sin(pi*(globalPos[1]+1.0/2.0))*sin(pi*(globalPos[2]+1.0/3.0)));
    }

    Dune::FieldVector<Scalar,dim> exactGrad (const GlobalPosition& globalPos) const
        {
        Dune::FieldVector<Scalar,dim> grad(0);
        double pi = 4.0*atan(1.0);
        grad[0] = pi*cos(pi*globalPos[0])*sin(pi*(globalPos[1]+1.0/2.0))*sin(pi*(globalPos[2]+1.0/3.0));
        grad[1] = pi*sin(pi*globalPos[0])*cos(pi*(globalPos[1]+1.0/2.0))*sin(pi*(globalPos[2]+1.0/3.0));
        grad[2] = pi*sin(pi*globalPos[0])*sin(pi*(globalPos[1]+1.0/2.0))*cos(pi*(globalPos[2]+1.0/3.0));

        return grad;
        }
private:
    Dumux::FVVelocity<TypeTag, typename GET_PROP_TYPE(TypeTag, Velocity) > velocity_;
    static constexpr Scalar eps_ = 1e-4;

};
} //end namespace

#endif