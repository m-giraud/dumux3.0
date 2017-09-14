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
 *
 * \brief Problem where air is injected under a low permeable layer in a depth of 2700m.
 */
#ifndef DUMUX_INJECTION_2P2C_PROBLEM_HH
#define DUMUX_INJECTION_2P2C_PROBLEM_HH

#include <dumux/porousmediumflow/2p2c/implicit/model.hh>
#include <dumux/porousmediumflow/implicit/problem.hh>
#include <dumux/material/fluidsystems/h2on2.hh>

#include "injection2pspatialparams.hh"

namespace Dumux
{

template <class TypeTag>
class Injection2p2cProblem;

namespace Properties
{
NEW_TYPE_TAG(Injection2p2cProblem, INHERITS_FROM(TwoPTwoC, InjectionSpatialParams));
NEW_TYPE_TAG(Injection2p2cBoxProblem, INHERITS_FROM(BoxModel, Injection2p2cProblem));
NEW_TYPE_TAG(Injection2p2pcCCProblem, INHERITS_FROM(CCModel, Injection2p2cProblem));

// Set the grid type
SET_TYPE_PROP(Injection2p2cProblem, Grid, Dune::YaspGrid<2>);

// Set the problem property
SET_TYPE_PROP(Injection2p2cProblem, Problem, Injection2p2cProblem<TypeTag>);

// Set fluid configuration
SET_TYPE_PROP(Injection2p2cProblem,
              FluidSystem,
              FluidSystems::H2ON2<typename GET_PROP_TYPE(TypeTag, Scalar), false /*useComplexRelations*/>);

// Define whether mole(true) or mass (false) fractions are used
SET_BOOL_PROP(Injection2p2cProblem, UseMoles, true);
}


/*!
 * \ingroup TwoPTwoCModel
 * \ingroup ImplicitTestProblems
 * \brief Problem where air is injected under a low permeable layer in a depth of 2700m.
 *
 * The domain is sized 60m times 40m and consists of two layers, a moderately
 * permeable one (\f$ K=10e-12\f$) for \f$ y<22m\f$ and one with a lower permeablility (\f$ K=10e-13\f$)
 * in the rest of the domain.
 *
 * A mixture of Nitrogen and Water vapor, which is composed according to the prevailing conditions (temperature, pressure)
 * enters a water-filled aquifer. This is realized with a solution-dependent Neumann boundary condition at the right boundary
 * (\f$ 7m<y<15m\f$). The aquifer is situated 2700m below sea level. The injected fluid phase migrates upwards due to buoyancy.
 * It accumulates and partially enters the lower permeable aquitard.
 *
 * The model is able to use either mole or mass fractions. The property useMoles can be set to either true or false in the
 * problem file. Make sure that the according units are used in the problem setup. The default setting for useMoles is true.
 *
 * This problem uses the \ref TwoPTwoCModel.
 *
 * To run the simulation execute the following line in shell:
 * <tt>./exercise1_2p2c -parameterFile exercise1.input>
 */
template <class TypeTag>
class Injection2p2cProblem : public ImplicitPorousMediaProblem<TypeTag>
{
    typedef ImplicitPorousMediaProblem<TypeTag> ParentType;

    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, FluidSystem) FluidSystem;

    enum {
        // Grid and world dimension
        dim = GridView::dimension,
        dimWorld = GridView::dimensionworld
    };

    // copy some indices for convenience
    typedef typename GET_PROP_TYPE(TypeTag, Indices) Indices;
    enum {
        wPhaseIdx = Indices::wPhaseIdx,
        nPhaseIdx = Indices::nPhaseIdx,


        wCompIdx = FluidSystem::wCompIdx,
        nCompIdx = FluidSystem::nCompIdx,

        contiH2OEqIdx = Indices::contiWEqIdx,
        contiN2EqIdx = Indices::contiNEqIdx
    };


    typedef typename GET_PROP_TYPE(TypeTag, PrimaryVariables) PrimaryVariables;
    typedef typename GET_PROP_TYPE(TypeTag, ElementVolumeVariables) ElementVolumeVariables;
    typedef typename GET_PROP_TYPE(TypeTag, BoundaryTypes) BoundaryTypes;
    typedef typename GET_PROP_TYPE(TypeTag, TimeManager) TimeManager;

    typedef typename GridView::template Codim<0>::Entity Element;
    typedef typename GridView::template Codim<dim>::Entity Vertex;
    typedef typename GridView::Intersection Intersection;

    typedef typename GET_PROP_TYPE(TypeTag, FVElementGeometry) FVElementGeometry;

    typedef Dune::FieldVector<Scalar, dimWorld> GlobalPosition;

    enum { isBox = GET_PROP_VALUE(TypeTag, ImplicitIsBox) };

    //! property that defines whether mole or mass fractions are used
    static const bool useMoles = GET_PROP_VALUE(TypeTag, UseMoles);

public:
    /*!
     * \brief The constructor
     *
     * \param timeManager The time manager
     * \param gridView The grid view
     */
    Injection2p2cProblem(TimeManager &timeManager,
                     const GridView &gridView)
        : ParentType(timeManager, gridView)
    {
        maxDepth_  = GET_RUNTIME_PARAM(TypeTag, Scalar, Problem.MaxDepth);

         // initialize the tables of the fluid system
        FluidSystem::init(/*tempMin=*/273.15,
                /*tempMax=*/423.15,
                /*numTemp=*/50,
                /*pMin=*/0.0,
                /*pMax=*/30e6,
                /*numP=*/300);

        //stateing in the console whether mole or mass fractions are used
        if(useMoles)
        {
            std::cout<<"problem uses mole-fractions"<<std::endl;
        }
        else
        {
            std::cout<<"problem uses mass-fractions"<<std::endl;
        }
    }

    /*!
     * \brief User defined output after the time integration
     *
     * Will be called diretly after the time integration.
     */
    void postTimeStep()
    {
        // Calculate storage terms
        PrimaryVariables storageW, storageN;
        this->model().globalPhaseStorage(storageW, wPhaseIdx);
        this->model().globalPhaseStorage(storageN, nPhaseIdx);

        // Write mass balance information for rank 0
        if (this->gridView().comm().rank() == 0) {
            std::cout<<"Storage: wetting=[" << storageW << "]"
                     << " nonwetting=[" << storageN << "]\n";
        }
    }


    /*!
     * \name Problem parameters
     */
    // \{

    /*!
     * \brief Returns the problem name
     *
     * This is used as a prefix for files generated by the simulation.
     */
    const std::string name() const
    { return "injection-2p2c"; }

    /*!
     * \brief Returns the temperature \f$ K \f$
     */
    Scalar temperature() const
    {
        return 273.15 + 30; // [K]
    }

    /*!
     * \brief Returns the source term
     *
     * \param values Stores the source values for the conservation equations in
     *               \f$ [ \textnormal{unit of primary variable} / (m^\textrm{dim} \cdot s )] \f$
     * \param globalPos The global position
     */
    void sourceAtPos(PrimaryVariables &values,
                     const GlobalPosition &globalPos) const
    {
        values = 0;
    }

    // \}

    /*!
     * \name Boundary conditions
     */
    // \{

    /*!
     * \brief Specifies which kind of boundary condition should be
     *        used for which equation on a given boundary segment
     *
     * \param values Stores the value of the boundary type
     * \param globalPos The global position
     */
    void boundaryTypesAtPos(BoundaryTypes &values,
                            const GlobalPosition &globalPos) const
    {
        if (globalPos[0] < eps_)
            values.setAllDirichlet();
        else
            values.setAllNeumann();
    }

    /*!
     * \brief Evaluates the boundary conditions for a Dirichlet
     *        boundary segment
     *
     * \param values Stores the Dirichlet values for the conservation equations in
     *               \f$ [ \textnormal{unit of primary variable} ] \f$
     * \param globalPos The global position
     */
    void dirichletAtPos(PrimaryVariables &values, const GlobalPosition &globalPos) const
    {
        initial_(values, globalPos);
    }

    /*!
     * \brief Evaluates the boundary conditions for a Neumann
     *        boundary segment in dependency on the current solution.
     *
     * \param values Stores the Neumann values for the conservation equations in
     *               \f$ [ \textnormal{unit of conserved quantity} / (m^(dim-1) \cdot s )] \f$
     * \param element The finite element
     * \param fvGeometry The finite volume geometry of the element
     * \param intersection The intersection between element and boundary
     * \param scvIdx The local index of the sub-control volume
     * \param boundaryFaceIdx The index of the boundary face
     * \param elemVolVars All volume variables for the element
     *
     * This method is used for cases, when the Neumann condition depends on the
     * solution and requires some quantities that are specific to the fully-implicit method.
     * The \a values store the mass flux of each phase normal to the boundary.
     * Negative values indicate an inflow.
     */
     void solDependentNeumann(PrimaryVariables &values,
                      const Element &element,
                      const FVElementGeometry &fvGeometry,
                      const Intersection &intersection,
                      const int scvIdx,
                      const int boundaryFaceIdx,
                      const ElementVolumeVariables &elemVolVars) const
    {
         values = 0;

         GlobalPosition globalPos;
         if (isBox)
             globalPos = element.geometry().corner(scvIdx);
         else
             globalPos = intersection.geometry().center();

         Scalar injectedPhaseMass = 1e-3;
         Scalar moleFracW = elemVolVars[scvIdx].moleFraction(nPhaseIdx, wCompIdx);
         if (globalPos[1] < 15 + eps_ && globalPos[1] > 7 -eps_) {
             values[contiN2EqIdx] = -(1-moleFracW)*injectedPhaseMass/FluidSystem::molarMass(nCompIdx); //mole/(m^2*s) -> kg/(s*m^2)
             values[contiH2OEqIdx] = -moleFracW*injectedPhaseMass/FluidSystem::molarMass(wCompIdx); //mole/(m^2*s) -> kg/(s*m^2)
         }
    }

    // \}

    /*!
     * \name Volume terms
     */
    // \{

    /*!
     * \brief Evaluates the initial values for a control volume
     *
     * \param values Stores the initial values for the conservation equations in
     *               \f$ [ \textnormal{unit of primary variables} ] \f$
     * \param globalPos The global position
     */
    void initialAtPos(PrimaryVariables &values, const GlobalPosition &globalPos) const
    {
        initial_(values, globalPos);
    }

    /*!
     * \brief Return the initial phase state inside a control volume.
     *
     * \param globalPos The global position
     */
    int initialPhasePresenceAtPos(const GlobalPosition &globalPos) const
    { return Indices::wPhaseOnly; }

    // \}

private:
    /*!
     * \brief Evaluates the initial values for a control volume
     *
     * The internal method for the initial condition
     *
     * \param values Stores the initial values for the conservation equations in
     *               \f$ [ \textnormal{unit of primary variables} ] \f$
     * \param globalPos The global position
     */
    void initial_(PrimaryVariables &values,
                  const GlobalPosition &globalPos) const
    {
        Scalar densityW = FluidSystem::H2O::liquidDensity(temperature(), 1e5);

        Scalar pl = 1e5 - densityW*this->gravity()[1]*(maxDepth_ - globalPos[1]);
        Scalar moleFracLiquidN2 = pl*0.95/BinaryCoeff::H2O_N2::henry(temperature());
        Scalar moleFracLiquidH2O = 1.0 - moleFracLiquidN2;

        Scalar meanM =
            FluidSystem::molarMass(wCompIdx)*moleFracLiquidH2O +
            FluidSystem::molarMass(nCompIdx)*moleFracLiquidN2;
        if(useMoles)
        {
            //mole-fraction formulation
            values[Indices::switchIdx] = moleFracLiquidN2;
        }
        else
        {
            //mass fraction formulation
            Scalar massFracLiquidN2 = moleFracLiquidN2*FluidSystem::molarMass(nCompIdx)/meanM;
            values[Indices::switchIdx] = massFracLiquidN2;
        }
        values[Indices::pressureIdx] = pl;
    }


    static constexpr Scalar eps_ = 1e-6;
    Scalar maxDepth_;

};
} //end namespace

#endif
