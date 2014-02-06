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
#ifndef DUMUX_2CSTOKES2P2CPROBLEM_HH
#define DUMUX_2CSTOKES2P2CPROBLEM_HH

#include <dune/grid/common/gridinfo.hh>
#include <dune/grid/multidomaingrid.hh>
#include <dune/grid/io/file/dgfparser.hh>

#include <dumux/multidomain/common/multidomainproblem.hh>
#include <dumux/multidomain/2cstokes2p2c/2cstokes2p2clocaloperator.hh>
#include <dumux/multidomain/2cstokes2p2c/2cstokes2p2cnewtoncontroller.hh>
#include <dumux/material/fluidsystems/h2oairfluidsystem.hh>
#include <dumux/linear/pardisobackend.hh>

#include "2cstokes2p2cspatialparams.hh"
#include "stokes2csubproblem.hh"
#include "2p2csubproblem.hh"

namespace Dumux
{
template <class TypeTag>
class TwoCStokesTwoPTwoCProblem;

namespace Properties
{
NEW_TYPE_TAG(TwoCStokesTwoPTwoCProblem, INHERITS_FROM(MultiDomain));

// Set the grid type
SET_PROP(TwoCStokesTwoPTwoCProblem, Grid)
{
 public:
#ifdef HAVE_UG
    typedef typename Dune::UGGrid<2> type;
#elif HAVE_ALUGRID
    typedef typename Dune::ALUGrid<2, 2, Dune::cube, Dune::nonconforming> type;
#endif
};

// Specify the multidomain gridview
SET_PROP(TwoCStokesTwoPTwoCProblem, GridView)
{
    typedef typename GET_PROP_TYPE(TypeTag, MultiDomainGrid) MDGrid;
 public:
    typedef typename MDGrid::LeafGridView type;
};

// Set the global problem
SET_TYPE_PROP(TwoCStokesTwoPTwoCProblem, Problem, Dumux::TwoCStokesTwoPTwoCProblem<TypeTag>);

// Set the two sub-problems of the global problem
SET_TYPE_PROP(TwoCStokesTwoPTwoCProblem, SubDomain1TypeTag, TTAG(Stokes2cSubProblem));
SET_TYPE_PROP(TwoCStokesTwoPTwoCProblem, SubDomain2TypeTag, TTAG(TwoPTwoCSubProblem));

// Set the global problem in the context of the two sub-problems
SET_TYPE_PROP(Stokes2cSubProblem, MultiDomainTypeTag, TTAG(TwoCStokesTwoPTwoCProblem));
SET_TYPE_PROP(TwoPTwoCSubProblem, MultiDomainTypeTag, TTAG(TwoCStokesTwoPTwoCProblem));

// Set the other sub-problem for each of the two sub-problems
SET_TYPE_PROP(Stokes2cSubProblem, OtherSubDomainTypeTag, TTAG(TwoPTwoCSubProblem));
SET_TYPE_PROP(TwoPTwoCSubProblem, OtherSubDomainTypeTag, TTAG(Stokes2cSubProblem));

SET_PROP(Stokes2cSubProblem, SpatialParams)
{ typedef Dumux::TwoCStokesTwoPTwoCSpatialParams<TypeTag> type; };
SET_PROP(TwoPTwoCSubProblem, SpatialParams)
{ typedef Dumux::TwoCStokesTwoPTwoCSpatialParams<TypeTag> type; };

// Set the fluid system
SET_PROP(TwoCStokesTwoPTwoCProblem, FluidSystem)
{
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef Dumux::FluidSystems::H2OAir<Scalar,
        Dumux::H2O<Scalar>,
        /*useComplexrelations=*/false> type;
};

SET_TYPE_PROP(TwoCStokesTwoPTwoCProblem, JacobianAssembler, Dumux::MultiDomainAssembler<TypeTag>);

// Set the local coupling operator
SET_TYPE_PROP(TwoCStokesTwoPTwoCProblem, MultiDomainCouplingLocalOperator, Dumux::TwoCStokesTwoPTwoCLocalOperator<TypeTag>);

SET_PROP(TwoCStokesTwoPTwoCProblem, SolutionVector)
{
 private:
    typedef typename GET_PROP_TYPE(TypeTag, MultiDomainGridOperator) MDGridOperator;
 public:
    typedef typename MDGridOperator::Traits::Domain type;
};

// newton controller
SET_TYPE_PROP(TwoCStokesTwoPTwoCProblem, NewtonController, Dumux::TwoCStokesTwoPTwoCNewtonController<TypeTag>);

// if you do not have PARDISO, the SuperLU solver is used:
#ifdef HAVE_PARDISO
SET_TYPE_PROP(TwoCStokesTwoPTwoCProblem, LinearSolver, PardisoBackend<TypeTag>);
#else
SET_TYPE_PROP(TwoCStokesTwoPTwoCProblem, LinearSolver, SuperLUBackend<TypeTag>);
#endif // HAVE_PARDISO

// set this to one here (must fit to the structure of the coupled matrix which has block length 1)
SET_INT_PROP(TwoCStokesTwoPTwoCProblem, NumEq, 1);

// Write the solutions of individual newton iterations?
SET_BOOL_PROP(TwoCStokesTwoPTwoCProblem, NewtonWriteConvergence, false);
}

/*!
 * \brief The problem class for the coupling of an isothermal two-component Stokes
 *        and an isothermal two-phase two-component Darcy model.
 *
 *        The problem class for the coupling of an isothermal two-component Stokes (stokes2c)
 *        and an isothermal two-phase two-component Darcy model (2p2c).
 *        It uses the 2p2cCoupling model and the Stokes2ccoupling model and provides
 *        the problem specifications for common parameters of the two submodels.
 *        The initial and boundary conditions of the submodels are specified in the two subproblems,
 *        2p2csubproblem.hh and stokes2csubproblem.hh, which are accessible via the coupled problem.
 */
template <class TypeTag = TTAG(TwoCStokesTwoPTwoCProblem) >
class TwoCStokesTwoPTwoCProblem : public MultiDomainProblem<TypeTag>
{
    template<int dim>
    struct VertexLayout
    {
        bool contains(Dune::GeometryType geomtype)
        { return geomtype.dim() == 0; }
    };

    typedef TwoCStokesTwoPTwoCProblem<TypeTag> ThisType;
    typedef MultiDomainProblem<TypeTag> ParentType;

    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, NewtonController) NewtonController;
    typedef typename GET_PROP_TYPE(TypeTag, TimeManager) TimeManager;

    typedef typename GET_PROP_TYPE(TypeTag, SubDomain1TypeTag) Stokes2cTypeTag;
    typedef typename GET_PROP_TYPE(TypeTag, SubDomain2TypeTag) TwoPTwoCTypeTag;

    typedef typename GET_PROP_TYPE(Stokes2cTypeTag, Problem) Stokes2cSubProblem;
    typedef typename GET_PROP_TYPE(TwoPTwoCTypeTag, Problem) TwoPTwoCSubProblem;

    typedef typename GET_PROP_TYPE(Stokes2cTypeTag, ElementSolutionVector) ElementSolutionVector1;
    typedef typename GET_PROP_TYPE(TwoPTwoCTypeTag, ElementSolutionVector) ElementSolutionVector2;

    typedef typename GET_PROP_TYPE(Stokes2cTypeTag, ElementVolumeVariables) ElementVolumeVariables1;
    typedef typename GET_PROP_TYPE(TwoPTwoCTypeTag, ElementVolumeVariables) ElementVolumeVariables2;

    typedef typename GET_PROP_TYPE(Stokes2cTypeTag, FluxVariables) BoundaryVariables1;

    typedef typename GET_PROP_TYPE(Stokes2cTypeTag, FVElementGeometry) FVElementGeometry1;
    typedef typename GET_PROP_TYPE(TwoPTwoCTypeTag, FVElementGeometry) FVElementGeometry2;

    typedef typename GET_PROP_TYPE(TypeTag, MultiDomainGrid) MDGrid;
    typedef typename MDGrid::LeafGridView MDGridView;
    enum { dim = MDGridView::dimension };

    typedef typename MDGrid::Traits::template Codim<0>::EntityPointer MDElementPointer;
    typedef typename MDGrid::Traits::template Codim<0>::Entity MDElement;
    typedef typename MDGrid::SubDomainGrid SDGrid;
    typedef typename SDGrid::template Codim<0>::EntityPointer SDElementPointer;

    typedef typename GET_PROP_TYPE(Stokes2cTypeTag, GridView) Stokes2cGridView;
    typedef typename GET_PROP_TYPE(TwoPTwoCTypeTag, GridView) TwoPTwoCGridView;
    typedef typename Stokes2cGridView::template Codim<0>::Entity SDElement1;
    typedef typename TwoPTwoCGridView::template Codim<0>::Entity SDElement2;

    typedef typename MDGrid::template Codim<0>::LeafIterator ElementIterator;
    typedef typename MDGrid::LeafSubDomainInterfaceIterator SDInterfaceIterator;

    typedef Dune::FieldVector<Scalar, dim> GlobalPosition;
    typedef Dune::FieldVector<Scalar, dim> FieldVector;

    typedef typename GET_PROP_TYPE(Stokes2cTypeTag, Indices) Stokes2cIndices;
    typedef typename GET_PROP_TYPE(TwoPTwoCTypeTag, Indices) TwoPTwoCIndices;

    typedef typename GET_PROP_TYPE(Stokes2cTypeTag, FluidSystem) FluidSystem;

    enum { numEq1 = GET_PROP_VALUE(Stokes2cTypeTag, NumEq) };
	enum { // indices in the Stokes domain
        momentumXIdx1 = Stokes2cIndices::momentumXIdx, //!< Index of the x-component of the momentum balance
        momentumYIdx1 = Stokes2cIndices::momentumYIdx, //!< Index of the y-component of the momentum balance
        momentumZIdx1 = Stokes2cIndices::momentumZIdx, //!< Index of the z-component of the momentum balance
        lastMomentumIdx1 = Stokes2cIndices::lastMomentumIdx, //!< Index of the last component of the momentum balance
        massBalanceIdx1 = Stokes2cIndices::massBalanceIdx, //!< Index of the mass balance
        transportEqIdx1 = Stokes2cIndices::transportEqIdx, //!< Index of the transport equation
	};

    enum { numEq2 = GET_PROP_VALUE(TwoPTwoCTypeTag, NumEq) };
	enum { // indices in the Darcy domain
        massBalanceIdx2 = TwoPTwoCIndices::pressureIdx,
        switchIdx2 = TwoPTwoCIndices::switchIdx,
        contiTotalMassIdx2 = TwoPTwoCIndices::contiNEqIdx,
        contiWEqIdx2 = TwoPTwoCIndices::contiWEqIdx,

        wCompIdx2 = TwoPTwoCIndices::wCompIdx,
        nCompIdx2 = TwoPTwoCIndices::nCompIdx,

        wPhaseIdx2 = TwoPTwoCIndices::wPhaseIdx,
        nPhaseIdx2 = TwoPTwoCIndices::nPhaseIdx

    };
	enum { phaseIdx = GET_PROP_VALUE(Stokes2cTypeTag, PhaseIdx) };
    enum { transportCompIdx1 = Stokes2cIndices::transportCompIdx };


public:
    /*!
     * \brief docme
     *
     * \param hostGrid docme
     * \param timeManager The time manager
     *
     */
    TwoCStokesTwoPTwoCProblem(MDGrid &mdGrid,
                              TimeManager &timeManager)
        : ParentType(mdGrid, timeManager)
    {
        try
        {
            // define location of the interface
            interface_ = GET_RUNTIME_PARAM_FROM_GROUP(TypeTag, Scalar, Grid, InterfacePos);
            noDarcyX_ = GET_RUNTIME_PARAM_FROM_GROUP(TypeTag, Scalar, Problem, NoDarcyX);
            episodeLength_ = GET_RUNTIME_PARAM_FROM_GROUP(TypeTag, Scalar, TimeManager, EpisodeLength);
            initializationTime_ = GET_RUNTIME_PARAM_FROM_GROUP(TypeTag, Scalar, TimeManager, InitTime);

            // define output options
            freqRestart_ = GET_RUNTIME_PARAM_FROM_GROUP(TypeTag, int, Output, FreqRestart);
            freqOutput_ = GET_RUNTIME_PARAM_FROM_GROUP(TypeTag, int, Output, FreqOutput);
            freqFluxOutput_ = GET_RUNTIME_PARAM_FROM_GROUP(TypeTag, int, Output, FreqFluxOutput);
            freqVaporFluxOutput_ = GET_RUNTIME_PARAM_FROM_GROUP(TypeTag, int, Output, FreqVaporFluxOutput);
        }
        catch (Dumux::ParameterException &e) {
            std::cerr << e << ". Abort!\n";
            exit(1) ;
        }
        catch (...) {
            std::cerr << "Unknown exception thrown!\n";
            exit(1);
        }

        stokes2c_ = this->sdID1();
        twoPtwoC_ = this->sdID2();

        initializeGrid();

        // initialize the tables of the fluid system
        FluidSystem::init(/*tempMin=*/273.15, /*tempMax=*/373.15, /*numTemp=*/200,
                          /*pMin=*/1e4, /*pMax=*/2e5, /*numP=*/200);

        if (initializationTime_ > 0.0)
            this->timeManager().startNextEpisode(initializationTime_);
        else
            this->timeManager().startNextEpisode(episodeLength_);
    }

    ~TwoCStokesTwoPTwoCProblem()
    {
        fluxFile_.close();
    };

    //! \copydoc Dumux::CoupledProblem::init()
    void init()
    {
        ParentType::init();

        // plot the Pc-Sw curves, if requested
        this->sdProblem2().spatialParams().plotMaterialLaw();

        std::cout << "Writing flux data at interface\n";
        if (this->timeManager().time() == 0)
        {
            fluxFile_.open("fluxes.out");
            fluxFile_ << "time; flux1; advFlux1; diffFlux1; totalFlux1; flux2; wPhaseFlux2; nPhaseFlux2\n";
            counter_ = 1;
        }
        else
            fluxFile_.open("fluxes.out", std::ios_base::app);
    }

    /*!
     * \brief docme
     */
    void initializeGrid()
    {
        MDGrid& mdGrid = this->mdGrid();
        mdGrid.startSubDomainMarking();

        // subdivide grid in two subdomains
        ElementIterator eendit = mdGrid.template leafend<0>();
        for (ElementIterator elementIt = mdGrid.template leafbegin<0>();
             elementIt != eendit; ++elementIt)
        {
            // this is required for parallelization
            // checks if element is within a partition
            if (elementIt->partitionType() != Dune::InteriorEntity)
                continue;

            GlobalPosition globalPos = elementIt->geometry().center();

            if (globalPos[1] > interface_)
                mdGrid.addToSubDomain(stokes2c_,*elementIt);
            else
                if(globalPos[0] > noDarcyX_)
                    mdGrid.addToSubDomain(twoPtwoC_,*elementIt);
        }
        mdGrid.preUpdateSubDomains();
        mdGrid.updateSubDomains();
        mdGrid.postUpdateSubDomains();

        gridinfo(this->sdGrid1());
        gridinfo(this->sdGrid2());

        this->sdProblem1().spatialParams().loadIntrinsicPermeability(this->sdGridView1());
        this->sdProblem2().spatialParams().loadIntrinsicPermeability(this->sdGridView2());
    }

    //! \copydoc Dumux::CoupledProblem::postTimeStep()
    void postTimeStep()
    {
        // call the postTimeStep function of the subproblems
        this->sdProblem1().postTimeStep();
        this->sdProblem2().postTimeStep();

        if (shouldWriteFluxFile() || shouldWriteVaporFlux())
        {
            counter_ = this->sdProblem1().currentVTKFileNumber() + 1;

            calculateFirstInterfaceFluxes();
            calculateSecondInterfaceFluxes();
        }
    }

    //! \copydoc Dumux::CoupledProblem::episodeEnd()
    void episodeEnd()
    { this->timeManager().startNextEpisode(episodeLength_); }

    /*
     * \brief Calculates fluxes and coupling terms at the interface for the Stokes model.
     *        Flux output files are created and the summarized flux is written to a file.
     */
    void calculateFirstInterfaceFluxes()
    {
        const MDGrid& mdGrid = this->mdGrid();
        ElementVolumeVariables1 elemVolVarsPrev1, elemVolVarsCur1;
        Scalar sumVaporFluxes = 0.;
        Scalar advectiveVaporFlux = 0.;
        Scalar diffusiveVaporFlux = 0.;

        // count number of elements to determine number of interface nodes
        int numElements = 0;
        const SDInterfaceIterator endIfIt = mdGrid.leafSubDomainInterfaceEnd(stokes2c_, twoPtwoC_);
        for (SDInterfaceIterator ifIt =
                 mdGrid.leafSubDomainInterfaceBegin(stokes2c_, twoPtwoC_); ifIt != endIfIt; ++ifIt)
            numElements++;

        const int numInterfaceVertices = numElements + 1;
        std::vector<InterfaceFluxes<numEq1> > outputVector(numInterfaceVertices); // vector for the output of the fluxes
        FVElementGeometry1 fvGeometry1;
        int interfaceVertIdx = -1;

        // loop over faces on the interface
        for (SDInterfaceIterator ifIt =
                 mdGrid.leafSubDomainInterfaceBegin(stokes2c_, twoPtwoC_); ifIt != endIfIt; ++ifIt)
        {
            const int firstFaceIdx = ifIt->indexInFirstCell();
            const MDElementPointer mdElementPointer1 = ifIt->firstCell(); // ATTENTION!!!
            const MDElement& mdElement1 = *mdElementPointer1;     // Entity pointer has to be copied before.
            const SDElementPointer sdElementPointer1 = this->sdElementPointer1(mdElement1);
            const SDElement1& sdElement1 = *sdElementPointer1;
            fvGeometry1.update(this->sdGridView1(), sdElement1);

            const Dune::GenericReferenceElement<typename MDGrid::ctype,dim>& referenceElement1 =
                Dune::GenericReferenceElements<typename MDGrid::ctype,dim>::general(mdElement1.type());
            const int numVerticesOfFace = referenceElement1.size(firstFaceIdx, 1, dim);

            // evaluate residual of the sub model without boundary conditions (stabilization is removed)
            // the element volume variables are updated here
            this->localResidual1().evalNoBoundary(sdElement1, fvGeometry1,
                                                  elemVolVarsPrev1, elemVolVarsCur1);

            for (int nodeInFace = 0; nodeInFace < numVerticesOfFace; nodeInFace++)
            {
                const int vertInElem1 = referenceElement1.subEntity(firstFaceIdx, 1, nodeInFace, dim);
                const FieldVector& vertexGlobal = mdElement1.geometry().corner(vertInElem1);
                const unsigned firstGlobalIdx = this->mdVertexMapper().map(stokes2c_, mdElement1, vertInElem1, dim);
                const ElementSolutionVector1& firstVertexDefect = this->localResidual1().residual();

                // loop over all interface vertices to check if vertex id is already in stack
                bool existing = false;
                for (int interfaceVertex=0; interfaceVertex < numInterfaceVertices; ++interfaceVertex)
                {
                    if (firstGlobalIdx == outputVector[interfaceVertex].globalIdx)
                    {
                        existing = true;
                        interfaceVertIdx = interfaceVertex;
                        break;
                    }
                }

                if (!existing)
                    interfaceVertIdx++;

                if (shouldWriteFluxFile()) // compute only if required
                {
                    outputVector[interfaceVertIdx].interfaceVertex = interfaceVertIdx;
                    outputVector[interfaceVertIdx].globalIdx = firstGlobalIdx;
                    outputVector[interfaceVertIdx].xCoord = vertexGlobal[0];
                    outputVector[interfaceVertIdx].yCoord = vertexGlobal[1];
                    outputVector[interfaceVertIdx].count += 1;
                    for (int eqIdx=0; eqIdx < numEq1; ++eqIdx)
                        outputVector[interfaceVertIdx].defect[eqIdx] +=
                            firstVertexDefect[vertInElem1][eqIdx];
                }

                // compute summarized fluxes for output
                if (shouldWriteVaporFlux())
                {
                	int boundaryFaceIdx =
                    	fvGeometry1.boundaryFaceIndex(firstFaceIdx, nodeInFace);

                	const BoundaryVariables1 boundaryVars1(this->sdProblem1(),
                                                       	   sdElement1,
                                                       	   fvGeometry1,
                                                           boundaryFaceIdx,
                                                           elemVolVarsCur1,
                                                           /*onBoundary=*/true);

                advectiveVaporFlux += computeAdvectiveVaporFluxes1(elemVolVarsCur1, boundaryVars1, vertInElem1);
                diffusiveVaporFlux += computeDiffusiveVaporFluxes1(elemVolVarsCur1, boundaryVars1, vertInElem1);
                sumVaporFluxes += firstVertexDefect[vertInElem1][transportEqIdx1];
				}
            }
        } // end loop over element faces on interface

        if (shouldWriteFluxFile())
        {
            std::cout << "Writing flux file\n";
            char outputname[20];
            sprintf(outputname, "%s%05d%s","fluxes1_", counter_,".out");
            std::ofstream outfile(outputname, std::ios_base::out);
            outfile << "Xcoord1 "
                    << "totalFlux1 "
                    << "componentFlux1 "
                    << "count1 "
                    << std::endl;
            for (int interfaceVertIdx=0; interfaceVertIdx < numInterfaceVertices; interfaceVertIdx++)
            {
                if (outputVector[interfaceVertIdx].count > 2)
                    std::cerr << "too often at one node!!";

                if (outputVector[interfaceVertIdx].count==2)
                    outfile << outputVector[interfaceVertIdx].xCoord << " "
                            << outputVector[interfaceVertIdx].defect[massBalanceIdx1] << " " // total mass flux
                            << outputVector[interfaceVertIdx].defect[transportEqIdx1] << " " // total flux of component
                            << outputVector[interfaceVertIdx].count << " "
                            << std::endl;
            }
            outfile.close();
        }
        if (shouldWriteVaporFlux())
            fluxFile_ << this->timeManager().time() + this->timeManager().timeStepSize() << "; "
                      << sumVaporFluxes << "; " << advectiveVaporFlux << "; " << diffusiveVaporFlux << "; "
                      << advectiveVaporFlux-diffusiveVaporFlux << "; ";
    }

    /*
     * \brief Calculates fluxes and coupling terms at the interface for the Darcy model.
     *        Flux output files are created and the summarized flux is written to a file.
     */
    void calculateSecondInterfaceFluxes()
    {
        const MDGrid& mdGrid = this->mdGrid();
        ElementVolumeVariables2 elemVolVarsPrev2, elemVolVarsCur2;

        Scalar sumVaporFluxes = 0.;
        Scalar sumWaterFluxInGasPhase = 0.;

        // count number of elements to determine number of interface nodes
        int numElements = 0;
        const SDInterfaceIterator endIfIt = mdGrid.leafSubDomainInterfaceEnd(stokes2c_, twoPtwoC_);
        for (SDInterfaceIterator ifIt =
                 mdGrid.leafSubDomainInterfaceBegin(stokes2c_, twoPtwoC_); ifIt != endIfIt; ++ifIt)
            numElements++;

        const int numInterfaceVertices = numElements + 1;
        std::vector<InterfaceFluxes<numEq2> > outputVector(numInterfaceVertices); // vector for the output of the fluxes
        FVElementGeometry2 fvGeometry2;
        int interfaceVertIdx = -1;

        // loop over the element faces on the interface
        for (SDInterfaceIterator ifIt =
                 mdGrid.leafSubDomainInterfaceBegin(stokes2c_, twoPtwoC_); ifIt != endIfIt; ++ifIt)
        {
            const int secondFaceIdx = ifIt->indexInSecondCell();
            const MDElementPointer mdElementPointer2 = ifIt->secondCell(); // ATTENTION!!!
            const MDElement& mdElement2 = *mdElementPointer2;     // Entity pointer has to be copied before.
            const SDElementPointer sdElementPointer2 = this->sdElementPointer2(mdElement2);
            const SDElement2& sdElement2 = *sdElementPointer2;
            fvGeometry2.update(this->sdGridView2(), sdElement2);

            const Dune::GenericReferenceElement<typename MDGrid::ctype,dim>& referenceElement2 =
                Dune::GenericReferenceElements<typename MDGrid::ctype,dim>::general(mdElement2.type());
            const int numVerticesOfFace = referenceElement2.size(secondFaceIdx, 1, dim);

            // evaluate residual of the sub model without boundary conditions
            this->localResidual2().evalNoBoundary(sdElement2, fvGeometry2,
                                                  elemVolVarsPrev2, elemVolVarsCur2);
            // evaluate the vapor fluxes within each phase
            this->localResidual2().evalPhaseFluxes();

            for (int nodeInFace = 0; nodeInFace < numVerticesOfFace; nodeInFace++)
            {
                const int vertInElem2 = referenceElement2.subEntity(secondFaceIdx, 1, nodeInFace, dim);
                const FieldVector& vertexGlobal = mdElement2.geometry().corner(vertInElem2);
                const unsigned secondGlobalIdx = this->mdVertexMapper().map(twoPtwoC_,mdElement2,vertInElem2,dim);
                const ElementSolutionVector2& secondVertexDefect = this->localResidual2().residual();

                bool existing = false;
                // loop over all interface vertices to check if vertex id is already in stack
                for (int interfaceVertex=0; interfaceVertex < numInterfaceVertices; ++interfaceVertex)
                {
                    if (secondGlobalIdx == outputVector[interfaceVertex].globalIdx)
                    {
                        existing = true;
                        interfaceVertIdx = interfaceVertex;
                        break;
                    }
                }

                if (!existing)
                    interfaceVertIdx++;

                if (shouldWriteFluxFile())
                {
                    outputVector[interfaceVertIdx].interfaceVertex = interfaceVertIdx;
                    outputVector[interfaceVertIdx].globalIdx = secondGlobalIdx;
                    outputVector[interfaceVertIdx].xCoord = vertexGlobal[0];
                    outputVector[interfaceVertIdx].yCoord = vertexGlobal[1];
                    for (int eqIdx=0; eqIdx < numEq2; ++eqIdx)
                        outputVector[interfaceVertIdx].defect[eqIdx] += secondVertexDefect[vertInElem2][eqIdx];
                    outputVector[interfaceVertIdx].count += 1;
                }
                if (shouldWriteVaporFlux())
                {
	                if (!existing) // add phase storage only once per vertex
    	                sumWaterFluxInGasPhase +=
        	                this->localResidual2().evalPhaseStorage(vertInElem2);

            	    sumVaporFluxes += secondVertexDefect[vertInElem2][contiWEqIdx2];
                	sumWaterFluxInGasPhase +=
                    	this->localResidual2().elementFluxes(vertInElem2);
          		}
            }
        }

        if (shouldWriteFluxFile())
        {
            char outputname[20];
            sprintf(outputname, "%s%05d%s","fluxes2_", counter_,".out");
            std::ofstream outfile(outputname, std::ios_base::out);
            outfile << "Xcoord2 "
                    << "totalFlux2 "
                    << "componentFlux2 "
                    << std::endl;

            for (int interfaceVertIdx=0; interfaceVertIdx < numInterfaceVertices; interfaceVertIdx++)
            {
                if (outputVector[interfaceVertIdx].count > 2)
                    std::cerr << "too often at one node!!";

                if (outputVector[interfaceVertIdx].count==2)
                    outfile << outputVector[interfaceVertIdx].xCoord << " "
                            << outputVector[interfaceVertIdx].defect[contiTotalMassIdx2] << " " // total mass flux
                            << outputVector[interfaceVertIdx].defect[contiWEqIdx2] << " " // total flux of component
                            << std::endl;
            }
            outfile.close();
        }
        if (shouldWriteVaporFlux()){
            Scalar sumWaterFluxInLiquidPhase = sumVaporFluxes - sumWaterFluxInGasPhase;
            fluxFile_ << sumVaporFluxes << "; "
                      << sumWaterFluxInLiquidPhase << "; "
                      << sumWaterFluxInGasPhase << std::endl;
        }
    }

    /*!
     * \brief docme
     *
     * \param elemVolVars1 docme
     * \param boundaryVars1 docme
     * \param vertInElem1 docme
     *
     */
    Scalar computeAdvectiveVaporFluxes1(const ElementVolumeVariables1& elemVolVars1,
                                        const BoundaryVariables1& boundaryVars1,
                                        int vertInElem1)
    {
        Scalar advFlux = elemVolVars1[vertInElem1].density() *
            elemVolVars1[vertInElem1].fluidState().massFraction(phaseIdx, transportCompIdx1) *
            boundaryVars1.normalVelocity();
        return advFlux;
    }

    /*!
     * \brief docme
     *
     * \param elemVolVars1 docme
     * \param boundaryVars1 docme
     * \param vertInElem1 docme
     *
     */
    Scalar computeDiffusiveVaporFluxes1(const ElementVolumeVariables1& elemVolVars1,
                                        const BoundaryVariables1& boundaryVars1,
                                        int vertInElem1)
    {
        Scalar diffFlux = boundaryVars1.moleFractionGrad(transportCompIdx1) *
            boundaryVars1.face().normal *
            boundaryVars1.diffusionCoeff(transportCompIdx1) *
            boundaryVars1.molarDensity() *
            FluidSystem::molarMass(transportCompIdx1);
        return diffFlux;
    }

    //! \copydoc Dumux::CoupledProblem::shouldWriteRestartFile()
    bool shouldWriteRestartFile() const
    {
        if ((this->timeManager().timeStepIndex() > 0 &&
             (this->timeManager().timeStepIndex() % freqRestart_ == 0))
            // also write a restart file at the end of each episode
            || this->timeManager().episodeWillBeOver())
            return true;
        return false;
    }


    //! \copydoc Dumux::CoupledProblem::shouldWriteOutput()
    bool shouldWriteOutput() const
    {
        if (this->timeManager().timeStepIndex() % freqOutput_ == 0
            || this->timeManager().episodeWillBeOver())
            return true;
        return false;
    }

    /*!
     * \brief Returns true if a file with the fluxes across the
     * free-flow -- porous-medium interface should be written.
     */
    bool shouldWriteFluxFile() const
    {
        if (this->timeManager().timeStepIndex() % freqFluxOutput_ == 0
            || this->timeManager().episodeWillBeOver())
            return true;
        return false;
    }

    /*!
     * \brief Returns true if the summarized vapor fluxes
     *        across the free-flow -- porous-medium interface,
     *        representing the evaporation rate (related to the
     *        interface area), should be written.
     */
    bool shouldWriteVaporFlux() const
    {
        if (this->timeManager().timeStepIndex() % freqVaporFluxOutput_ == 0
            || this->timeManager().episodeWillBeOver())
            return true;
        return false;

    }

    Stokes2cSubProblem& stokes2cProblem()
    { return this->sdProblem1(); }
    const Stokes2cSubProblem& stokes2cProblem() const
    { return this->sdProblem1(); }

    TwoPTwoCSubProblem& twoPtwoCProblem()
    { return this->sdProblem2(); }
    const TwoPTwoCSubProblem& twoPtwoCProblem() const
    { return this->sdProblem2(); }

private:
    typename MDGrid::SubDomainType stokes2c_;
    typename MDGrid::SubDomainType twoPtwoC_;

    unsigned counter_;
    unsigned freqRestart_;
    unsigned freqOutput_;
    unsigned freqFluxOutput_;
    unsigned freqVaporFluxOutput_;

    Scalar interface_;
    Scalar noDarcyX_;
    Scalar episodeLength_;
    Scalar initializationTime_;

    template <int numEq>
    struct InterfaceFluxes
    {
        unsigned count;
        unsigned interfaceVertex;
        unsigned globalIdx;
        Scalar xCoord;
        Scalar yCoord;
        Dune::FieldVector<Scalar, numEq> defect;

        InterfaceFluxes()
        {
            count = 0;
            interfaceVertex = 0;
            globalIdx = 0;
            xCoord = 0.0;
            yCoord = 0.0;
            defect = 0.0;
        }
    };
    std::ofstream fluxFile_;
};

} //end namespace

#endif