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
 * \brief Definition of a problem, where CO2 is injected under a reservoir.
 */
#ifndef DUMUX_HETEROGENEOUS_NI_PROBLEM_NI_HH
#define DUMUX_HETEROGENEOUS_NI_PROBLEM_NI_HH

#include <dune/common/version.hh>

#if HAVE_ALUGRID
#include <dune/grid/alugrid/2d/alugrid.hh>
#else
#warning ALUGrid is necessary for this test.
#endif

#include <dune/grid/io/file/dgfparser/dgfs.hh>
#include <dumux/implicit/co2ni/co2nimodel.hh>
#include <dumux/implicit/co2ni/co2nivolumevariables.hh>
#include <dumux/material/fluidsystems/brineco2fluidsystem.hh>
#include <dumux/implicit/common/implicitporousmediaproblem.hh>
#include <dumux/implicit/box/intersectiontovertexbc.hh>
#include <test/implicit/co2/heterogeneousspatialparameters.hh>

#include "heterogeneousco2tables.hh"

#define ISOTHERMAL 0

namespace Dumux
{

template <class TypeTag>
class HeterogeneousNIProblem;

namespace Properties
{
NEW_TYPE_TAG(HeterogeneousNIProblem, INHERITS_FROM(TwoPTwoCNI, HeterogeneousSpatialParams));
NEW_TYPE_TAG(HeterogeneousNIBoxProblem, INHERITS_FROM(BoxModel, HeterogeneousNIProblem));
NEW_TYPE_TAG(HeterogeneousNICCProblem, INHERITS_FROM(CCModel, HeterogeneousNIProblem));


// Set the grid type
#if HAVE_ALUGRID
SET_TYPE_PROP(HeterogeneousNIProblem, Grid, Dune::ALUGrid<2, 2, Dune::cube, Dune::nonconforming>);
#else
SET_TYPE_PROP(HeterogeneousNIProblem, Grid, Dune::YaspGrid<2>);
#endif

// Set the problem property
SET_PROP(HeterogeneousNIProblem, Problem)
{
    typedef Dumux::HeterogeneousNIProblem<TypeTag> type;
};

// Set fluid configuration
SET_PROP(HeterogeneousNIProblem, FluidSystem)
{
    typedef Dumux::BrineCO2FluidSystem<TypeTag> type;
};

// Set the CO2 table to be used; in this case not the the default table
SET_TYPE_PROP(HeterogeneousNIProblem, CO2Table, Dumux::Heterogeneous::CO2Tables);
// Set the salinity mass fraction of the brine in the reservoir
SET_SCALAR_PROP(HeterogeneousNIProblem, ProblemSalinity, 1e-1);

//! the CO2 Model and VolumeVariables properties
SET_TYPE_PROP(HeterogeneousNIProblem, Model, CO2NIModel<TypeTag>);
SET_TYPE_PROP(HeterogeneousNIProblem, VolumeVariables, CO2NIVolumeVariables<TypeTag>);

// Enable gravity
SET_BOOL_PROP(HeterogeneousNIProblem, ProblemEnableGravity, true);

SET_BOOL_PROP(HeterogeneousNIProblem, ImplicitEnableJacobianRecycling, false);
SET_BOOL_PROP(HeterogeneousNIProblem, VtkAddVelocity, false);

SET_BOOL_PROP(HeterogeneousNIProblem, UseMoles, false);
}


/*!
 * \ingroup CO2NIModel
 * \ingroup ImplicitTestProblems
 * \brief Problem where CO2 is injected under a low permeable layer in a depth of 1200m.
 *
 * The domain is sized 200m times 100m and consists of four layers, a
 * permeable reservoir layer at the bottom, a barrier rock layer with reduced permeability followed by another reservoir layer
 * and at the top a barrier rock layer with a very low permeablility.
 *
 * CO2 is injected at the permeable bottom layer
 * from the left side. The domain is initially filled with brine.
 *
 * The grid is unstructered and permeability and porosity for the elements are read in from the grid file. The grid file
 * also contains so-called boundary ids which can be used assigned during the grid creation in order to differentiate
 * between different parts of the boundary.
 * These boundary ids can be imported into the problem where the boundary conditions can then be assigned accordingly.
 *
 * The model is able to use either mole or mass fractions. The property useMoles can be set to either true or false in the
 * problem file. Make sure that the according units are used in the problem setup. The default setting for useMoles is false.
 *
 * To run the simulation execute the following line in shell (works with the box and cell centered spatial discretization method):
 * <tt>./test_ccco2ni </tt> or <tt>./test_boxco2ni </tt>
 */
template <class TypeTag >
class HeterogeneousNIProblem : public ImplicitPorousMediaProblem<TypeTag>
{
    typedef ImplicitPorousMediaProblem<TypeTag> ParentType;

    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;
    typedef typename GET_PROP_TYPE(TypeTag, Grid) Grid;
    typedef Dune::GridPtr<Grid> GridPointer;
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, FluidSystem) FluidSystem;
    typedef typename GET_PROP_TYPE(TypeTag, VolumeVariables) VolumeVariables;

    static const bool useMoles = GET_PROP_VALUE(TypeTag, UseMoles);

    enum {
        // Grid and world dimension
        dim = GridView::dimension,
        dimWorld = GridView::dimensionworld
    };

    // copy some indices for convenience
    typedef typename GET_PROP_TYPE(TypeTag, Indices) Indices;
    enum {
        lPhaseIdx = Indices::wPhaseIdx,
        gPhaseIdx = Indices::nPhaseIdx,


        BrineIdx = FluidSystem::BrineIdx,
        CO2Idx = FluidSystem::CO2Idx,

        conti0EqIdx = Indices::conti0EqIdx,
        contiCO2EqIdx = conti0EqIdx + CO2Idx,
#if !ISOTHERMAL
        temperatureIdx = CO2Idx +1,
        energyEqIdx = contiCO2EqIdx +1,
#endif

    };


    typedef typename GET_PROP_TYPE(TypeTag, PrimaryVariables) PrimaryVariables;
    typedef typename GET_PROP_TYPE(TypeTag, BoundaryTypes) BoundaryTypes;
    typedef typename GET_PROP_TYPE(TypeTag, TimeManager) TimeManager;

    typedef typename GridView::template Codim<0>::Entity Element;
    typedef typename GridView::template Codim<0>::Iterator ElementIterator;
    typedef typename GridView::template Codim<dim>::Entity Vertex;
    typedef typename GridView::Intersection Intersection;

    typedef typename GET_PROP_TYPE(TypeTag, FVElementGeometry) FVElementGeometry;
    typedef typename GET_PROP_TYPE(TypeTag, GridCreator) GridCreator;

    typedef Dune::FieldVector<Scalar, dimWorld> GlobalPosition;
    typedef typename GET_PROP_TYPE(TypeTag, PTAG(CO2Table)) CO2Table;
    typedef Dumux::CO2<Scalar, CO2Table> CO2;
    enum { isBox = GET_PROP_VALUE(TypeTag, ImplicitIsBox) };
    enum { dofCodim = isBox ? dim : 0 };

public:
    /*!
     * \brief The constructor
     *
     * \param timeManager The time manager
     * \param gridView The grid view
     */
    HeterogeneousNIProblem(TimeManager &timeManager,
                     const GridView &gridView)
#if DUNE_VERSION_NEWER(DUNE_COMMON, 2, 3)
        : ParentType(timeManager, GridCreator::grid().leafGridView()),
#else
        : ParentType(timeManager, GridCreator::grid().leafView()),
#endif
          //Boundary Id Setup:
          injectionTop_ (1),
          injectionBottom_(2),
          dirichletBoundary_(3),
          noFlowBoundary_(4),
          intersectionToVertexBC_(*this)
    {
        try
        {
            nTemperature_       = GET_RUNTIME_PARAM(TypeTag, int, FluidSystem.NTemperature);
            nPressure_          = GET_RUNTIME_PARAM(TypeTag, int, FluidSystem.NPressure);
            pressureLow_        = GET_RUNTIME_PARAM(TypeTag, Scalar, FluidSystem.PressureLow);
            pressureHigh_       = GET_RUNTIME_PARAM(TypeTag, Scalar, FluidSystem.PressureHigh);
            temperatureLow_     = GET_RUNTIME_PARAM(TypeTag, Scalar, FluidSystem.TemperatureLow);
            temperatureHigh_    = GET_RUNTIME_PARAM(TypeTag, Scalar, FluidSystem.TemperatureHigh);
            depthBOR_           = GET_RUNTIME_PARAM(TypeTag, Scalar, Problem.DepthBOR);
            name_               = GET_RUNTIME_PARAM(TypeTag, std::string, Problem.Name);
            injectionRate_      = GET_RUNTIME_PARAM(TypeTag, Scalar, Problem.InjectionRate);
            injectionPressure_ = GET_RUNTIME_PARAM(TypeTag, Scalar, Problem.InjectionPressure);
            injectionTemperature_ = GET_RUNTIME_PARAM(TypeTag, Scalar, Problem.InjectionTemperature);
        }
        catch (Dumux::ParameterException &e) {
            std::cerr << e << ". Abort!\n";
            exit(1) ;
        }
        catch (...) {
            std::cerr << "Unknown exception thrown!\n";
            exit(1);
        }

        /* Alternative syntax:
         * typedef typename GET_PROP(TypeTag, ParameterTree) ParameterTree;
         * const Dune::ParameterTree &tree = ParameterTree::tree();
         * nTemperature_       = tree.template get<int>("FluidSystem.nTemperature");
         *
         * + We see what we do
         * - Reporting whether it was used does not work
         * - Overwriting on command line not possible
        */

        GridPointer *gridPtr = &GridCreator::gridPtr();
        this->spatialParams().setParams(gridPtr);



        eps_ = 1e-6;

        // initialize the tables of the fluid system
        //FluidSystem::init();
        FluidSystem::init(/*Tmin=*/temperatureLow_,
                          /*Tmax=*/temperatureHigh_,
                          /*nT=*/nTemperature_,
                          /*pmin=*/pressureLow_,
                          /*pmax=*/pressureHigh_,
                          /*np=*/nPressure_);

        //stateing in the console whether mole or mass fractions are used
        if(!useMoles)
        {
        	std::cout<<"problem uses mass-fractions"<<std::endl;
        }
        else
        {
        	std::cout<<"problem uses mole-fractions"<<std::endl;
        }
    }

    /*!
     * \brief Called directly after the time integration.
     */
    void postTimeStep()
    {
        // Calculate storage terms
        PrimaryVariables storageL, storageG;
        this->model().globalPhaseStorage(storageL, lPhaseIdx);
        this->model().globalPhaseStorage(storageG, gPhaseIdx);

        // Write mass balance information for rank 0
        if (this->gridView().comm().rank() == 0) {
            std::cout<<"Storage: liquid=[" << storageL << "]"
                     << " gas=[" << storageG << "]\n";
        }
    }

    /*!
     * \brief Add enthalpy and peremeability to output.
     */
    void addOutputVtkFields()
        {
            typedef Dune::BlockVector<Dune::FieldVector<double, 1> > ScalarField;

            // get the number of degrees of freedom
            unsigned numDofs = this->model().numDofs();
            unsigned numElements = this->gridView().size(0);

            //create required scalar fields
            ScalarField *Kxx = this->resultWriter().allocateManagedBuffer(numElements);
            ScalarField *cellPorosity = this->resultWriter().allocateManagedBuffer(numElements);
            ScalarField *boxVolume = this->resultWriter().allocateManagedBuffer(numDofs);
            ScalarField *enthalpyW = this->resultWriter().allocateManagedBuffer(numDofs);
            ScalarField *enthalpyN = this->resultWriter().allocateManagedBuffer(numDofs);
            (*boxVolume) = 0;

            //Fill the scalar fields with values

            ScalarField *rank = this->resultWriter().allocateManagedBuffer(numElements);

            FVElementGeometry fvGeometry;
            VolumeVariables volVars;

            ElementIterator eIt = this->gridView().template begin<0>();
            ElementIterator eEndIt = this->gridView().template end<0>();
            for (; eIt != eEndIt; ++eIt)
            {
                int eIdx = this->elementMapper().map(*eIt);
                (*rank)[eIdx] = this->gridView().comm().rank();
                fvGeometry.update(this->gridView(), *eIt);


                for (int scvIdx = 0; scvIdx < fvGeometry.numScv; ++scvIdx)
                {
                    int globalIdx = this->model().dofMapper().map(*eIt, scvIdx, dofCodim);
                    volVars.update(this->model().curSol()[globalIdx],
                                   *this,
                                   *eIt,
                                   fvGeometry,
                                   scvIdx,
                                   false);
                    (*boxVolume)[globalIdx] += fvGeometry.subContVol[scvIdx].volume;
                    (*enthalpyW)[globalIdx] = volVars.enthalpy(lPhaseIdx);
                    (*enthalpyN)[globalIdx] = volVars.enthalpy(gPhaseIdx);
                }
                (*Kxx)[eIdx] = this->spatialParams().intrinsicPermeability(*eIt, fvGeometry, /*element data*/ 0);
                (*cellPorosity)[eIdx] = this->spatialParams().porosity(*eIt, fvGeometry, /*element data*/ 0);
            }

            //pass the scalar fields to the vtkwriter
            this->resultWriter().attachDofData(*boxVolume, "boxVolume", isBox);
            this->resultWriter().attachDofData(*Kxx, "Kxx", false); //element data
            this->resultWriter().attachDofData(*cellPorosity, "cellwisePorosity", false); //element data
            this->resultWriter().attachDofData(*enthalpyW, "enthalpyW", isBox);
            this->resultWriter().attachDofData(*enthalpyN, "enthalpyN", isBox);

        }

    /*!
     * \brief The problem name.
     *
     * This is used as a prefix for files generated by the simulation.
     */
    const std::string name() const
    { return name_; }

#if ISOTHERMAL
    /*!
     * \brief Returns the temperature within the domain.
     *
     * \param globalPos The position
     *
     * This problem assumes a geothermal gradient with 
     * a surface temperature of 10 degrees Celsius.
     */
    Scalar temperatureAtPos(const GlobalPosition &globalPos) const
    {
        return temperature_(globalPos);
    };
#endif

    /*!
     * \brief Returns the sources within the domain.
     *
     * \param values Stores the source values, acts as return value
     * \param globalPos The global position
     *
     * Depending on whether useMoles is set on true or false, the flux has to be given either in
     * kg/(m^3*s) or mole/(m^3*s) in the input file!!
     *
     * Note that the energy balance is always calculated in terms of specific enthalpies [J/kg]
     * and that the Neumann fluxes have to be specified accordingly.
     */
    void sourceAtPos(PrimaryVariables &values,
                const GlobalPosition &globalPos) const
    {
        values = 0;
    }

    /*!
     * \name Boundary conditions
     */

    /*!
     * \brief Specifies which kind of boundary condition should be
     *        used for which equation on a given boundary segment.
     *
     * \param values The boundary types for the conservation equations
     * \param vertex The vertex for which the boundary type is set
     */

    void boundaryTypes(BoundaryTypes &values, const Vertex &vertex) const
    {
        intersectionToVertexBC_.boundaryTypes(values, vertex);
    }

    /*!
     * \brief Specifies which kind of boundary condition should be
     *        used for which equation on a given boundary segment.
     *
     * \param values The boundary types for the conservation equations
     * \param intersection specifies the intersection at which boundary
     *           condition is to set
     */
    void boundaryTypes(BoundaryTypes &values, const Intersection &intersection) const
    {
        int boundaryId = intersection.boundaryId();
        if (boundaryId < 1 || boundaryId > 4)
        {
            std::cout<<"invalid boundaryId: "<<boundaryId<<std::endl;
            DUNE_THROW(Dune::InvalidStateException, "Invalid " << boundaryId);
        }
        if (boundaryId == dirichletBoundary_)
            values.setAllDirichlet();
        else
            values.setAllNeumann();
    }

    /*!
     * \brief Evaluate the boundary conditions for a dirichlet
     *        boundary segment.
     *
     * \param values The dirichlet values for the primary variables
     * \param globalPos The global position
     *
     * For this method, the \a values parameter stores primary variables.
     */
    void dirichletAtPos(PrimaryVariables &values, const GlobalPosition &globalPos) const
    {
        initial_(values, globalPos);
    }

    /*!
     * \brief Evaluate the boundary conditions for a neumann
     *        boundary segment.
     *
     * \param values The neumann values for the conservation equations
     * \param element The finite element
     * \param fvGeometry The finite-volume geometry in the box scheme
     * \param intersection The intersection between element and boundary
     * \param scvIdx The local vertex index
     * \param boundaryFaceIdx The index of the boundary face
     *
     * For this method, the \a values parameter stores the mass flux
     * in normal direction of each phase. Negative values mean influx.
     *
     * Depending on whether useMoles is set on true or false, the flux has to be given either in
     * kg/(m^2*s) or mole/(m^2*s) in the input file!!
     * Note that the energy balance is always calculated in terms of specific enthalpies [J/kg]
     * and that the Neumann fluxes have to be specified accordingly.
     */
    void neumann(PrimaryVariables &values,
                 const Element &element,
                 const FVElementGeometry &fvGeometry,
                 const Intersection &intersection,
                 int scvIdx,
                 int boundaryFaceIdx) const
    {
        int boundaryId = intersection.boundaryId();

        values = 0;
        if (boundaryId == injectionBottom_)
        {
            values[contiCO2EqIdx] = -injectionRate_; ///FluidSystem::molarMass(CO2Idx); // kg/(s*m^2) or mole/(m^2*s) !!
#if !ISOTHERMAL
            values[energyEqIdx] = -injectionRate_/*kg/(m^2 s)*/*CO2::gasEnthalpy(
                                    injectionTemperature_, injectionPressure_)/*J/kg*/; // W/(m^2)
#endif
        }
    }

    // \}

    /*!
     * \name Volume terms
     */
    // \{

    /*!
     * \brief Evaluate the initial value for a control volume.
     *
     * \param values The initial values for the primary variables
     * \param globalPos The center of the finite volume which ought to be set.
     *
     * For this method, the \a values parameter stores primary
     * variables.
     */
    void initialAtPos(PrimaryVariables &values,
                      const GlobalPosition &globalPos) const
    {
        initial_(values, globalPos);
    }

    /*!
     * \brief Return the initial phase state inside a control volume.
     *
     * \param vertex The vertex
     * \param globalIdx The index of the global vertex
     * \param globalPos The global position
     */
    int initialPhasePresence(const Vertex &vertex,
                             int &globalIdx,
                             const GlobalPosition &globalPos) const
    { return Indices::wPhaseOnly; }

    // \}

private:
    // the internal method for the initial condition
    void initial_(PrimaryVariables &values,
                  const GlobalPosition &globalPos) const
    {
        Scalar temp = temperature_(globalPos);
        Scalar densityW = FluidSystem::Brine::liquidDensity(temp, 1e7);

        Scalar pl =  1e5 - densityW*this->gravity()[dim-1]*(depthBOR_ - globalPos[dim-1]);
        Scalar moleFracLiquidCO2 = 0.00;
        Scalar moleFracLiquidBrine = 1.0 - moleFracLiquidCO2;

        Scalar meanM =
            FluidSystem::molarMass(BrineIdx)*moleFracLiquidBrine +
            FluidSystem::molarMass(CO2Idx)*moleFracLiquidCO2;

        Scalar massFracLiquidCO2 = moleFracLiquidCO2*FluidSystem::molarMass(CO2Idx)/meanM;

        values[Indices::pressureIdx] = pl;
        values[Indices::switchIdx] = massFracLiquidCO2;
#if !ISOTHERMAL
            values[temperatureIdx] = temperature_(globalPos); //K
#endif


    }

    Scalar temperature_(const GlobalPosition globalPos) const
    {
        Scalar T = 283.0 + (depthBOR_ - globalPos[dim-1])*0.03; 
        return T;
    };

    Scalar depthBOR_;
    Scalar injectionRate_;
    Scalar injectionPressure_;
    Scalar injectionTemperature_;
    Scalar eps_;

    int nTemperature_;
    int nPressure_;

    std::string name_ ;

    Scalar pressureLow_, pressureHigh_;
    Scalar temperatureLow_, temperatureHigh_;

    int injectionTop_;
    int injectionBottom_;
    int dirichletBoundary_;
    int noFlowBoundary_;

    const IntersectionToVertexBC<TypeTag> intersectionToVertexBC_;
};
} //end namespace

#endif
