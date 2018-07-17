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
 * \brief Test for the 1d-3d embedded mixed-dimension model coupling two
 *        one-phase porous medium flow problems
 */
#include <config.h>

#include <ctime>
#include <iostream>
#include <fstream>

#include <dune/common/parallel/mpihelper.hh>
#include <dune/common/timer.hh>
#include <dune/istl/io.hh>

#include <dumux/common/properties.hh>
#include <dumux/common/parameters.hh>
#include <dumux/common/dumuxmessage.hh>
#include <dumux/common/geometry/diameter.hh>
#include <dumux/linear/seqsolverbackend.hh>
#include <dumux/assembly/fvassembler.hh>
#include <dumux/assembly/diffmethod.hh>
#include <dumux/discretization/methods.hh>
#include <dumux/io/vtkoutputmodule.hh>
#include <dumux/io/grid/gridmanager.hh>

#include <dumux/multidomain/traits.hh>
#include <dumux/multidomain/fvassembler.hh>
#include <dumux/multidomain/newtonsolver.hh>
#include <dumux/multidomain/embedded/couplingmanager2d3d.hh>

#include "matrixproblem.hh"
#include "fractureproblem.hh"

namespace Dumux {
namespace Properties {

SET_PROP(MatrixTypeTag, CouplingManager)
{
    using Traits = MultiDomainTraits<TypeTag, TTAG(FractureTypeTag)>;
    using type = EmbeddedCouplingManager2d3d<Traits>;
};

SET_PROP(FractureTypeTag, CouplingManager)
{
    using Traits = MultiDomainTraits<TTAG(MatrixTypeTag), TypeTag>;
    using type = EmbeddedCouplingManager2d3d<Traits>;
};

SET_TYPE_PROP(MatrixTypeTag, PointSource, typename GET_PROP_TYPE(TypeTag, CouplingManager)::PointSourceTraits::template PointSource<0>);
SET_TYPE_PROP(FractureTypeTag, PointSource, typename GET_PROP_TYPE(TypeTag, CouplingManager)::PointSourceTraits::template PointSource<1>);
SET_TYPE_PROP(MatrixTypeTag, PointSourceHelper, typename GET_PROP_TYPE(TypeTag, CouplingManager)::PointSourceTraits::template PointSourceHelper<0>);
SET_TYPE_PROP(FractureTypeTag, PointSourceHelper, typename GET_PROP_TYPE(TypeTag, CouplingManager)::PointSourceTraits::template PointSourceHelper<1>);

} // end namespace Properties

struct SolverTag { struct Nonlinear {}; struct Linear {}; };

template<class Assembler, class LinearSolver, class CouplingManager, class SolutionVector>
void assembleSolveUpdate(Assembler& assembler, LinearSolver& linearSolver, CouplingManager& couplingManager, SolutionVector& sol, SolverTag::Nonlinear)
{
    // the non-linear solver
    using NewtonSolver = MultiDomainNewtonSolver<typename Assembler::element_type, typename LinearSolver::element_type, typename CouplingManager::element_type>;
    NewtonSolver nonLinearSolver(assembler, linearSolver, couplingManager);

    // solve the non-linear system
    nonLinearSolver.solve(sol);
}

template<class Assembler, class LinearSolver, class CouplingManager, class SolutionVector>
void assembleSolveUpdate(Assembler& assembler, LinearSolver& linearSolver, CouplingManager& couplingManager, SolutionVector& sol, SolverTag::Linear)
{
    Dune::Timer assembleTimer(false), solveTimer(false), updateTimer(false);
    std::cout << "\nAssembling linear system... " << std::flush;

    // assemble stiffness matrix
    assembleTimer.start();
    couplingManager->updateSolution(sol);
    assembler->assembleJacobianAndResidual(sol);
    assembleTimer.stop();

    std::cout << "done.\n";
    std::cout << "Solving linear system ("
              << linearSolver->name() << ") ... " << std::flush;

    // solve linear system
    solveTimer.start();
    auto deltaSol = sol;
    const bool converged = linearSolver->template solve<2>(assembler->jacobian(), deltaSol, assembler->residual());
    if (!converged) DUNE_THROW(Dune::MathError, "Linear solver did not converge!");
    solveTimer.stop();

    // update variables
    updateTimer.start();
    sol -= deltaSol;
    couplingManager->updateSolution(sol);
    assembler->updateGridVariables(sol);
    updateTimer.stop();

    std::cout << "done.\n";
    const auto elapsedTot = assembleTimer.elapsed() + solveTimer.elapsed() + updateTimer.elapsed();
    std::cout << "Assemble/solve/update time: "
              <<  assembleTimer.elapsed() << "(" << 100*assembleTimer.elapsed()/elapsedTot << "%)/"
              <<  solveTimer.elapsed() << "(" << 100*solveTimer.elapsed()/elapsedTot << "%)/"
              <<  updateTimer.elapsed() << "(" << 100*updateTimer.elapsed()/elapsedTot << "%)"
              << "\n";
}

} // end namespace Dumux

int main(int argc, char** argv) try
{
    using namespace Dumux;

    // initialize MPI, finalize is done automatically on exit
    const auto& mpiHelper = Dune::MPIHelper::instance(argc, argv);

    // print dumux start message
    if (mpiHelper.rank() == 0)
        DumuxMessage::print(/*firstCall=*/true);

    // parse command line arguments and input file
    Parameters::init(argc, argv);

    // Define the sub problem type tags
    using BulkTypeTag = TTAG(MatrixTypeTag);
    using LowDimTypeTag = TTAG(FractureTypeTag);

    // try to create a grid (from the given grid file or the input file)
    // for both sub-domains
    using BulkGridManager = Dumux::GridManager<typename GET_PROP_TYPE(BulkTypeTag, Grid)>;
    BulkGridManager bulkGridManager;
    bulkGridManager.init("Matrix"); // pass parameter group

    using LowDimGridManager = Dumux::GridManager<typename GET_PROP_TYPE(LowDimTypeTag, Grid)>;
    LowDimGridManager lowDimGridManager;
    lowDimGridManager.init("Fracture"); // pass parameter group

    ////////////////////////////////////////////////////////////
    // run instationary non-linear problem on this grid
    ////////////////////////////////////////////////////////////

    // we compute on the leaf grid view
    const auto& bulkGridView = bulkGridManager.grid().leafGridView();
    const auto& lowDimGridView = lowDimGridManager.grid().leafGridView();

    // create the finite volume grid geometry
    using BulkFVGridGeometry = typename GET_PROP_TYPE(BulkTypeTag, FVGridGeometry);
    auto bulkFvGridGeometry = std::make_shared<BulkFVGridGeometry>(bulkGridView);
    bulkFvGridGeometry->update();
    using LowDimFVGridGeometry = typename GET_PROP_TYPE(LowDimTypeTag, FVGridGeometry);
    auto lowDimFvGridGeometry = std::make_shared<LowDimFVGridGeometry>(lowDimGridView);
    lowDimFvGridGeometry->update();

    // the mixed dimension type traits
    using Traits = MultiDomainTraits<BulkTypeTag, LowDimTypeTag>;
    constexpr auto bulkIdx = Traits::template DomainIdx<0>();
    constexpr auto lowDimIdx = Traits::template DomainIdx<1>();

    // the coupling manager
    using CouplingManager = typename GET_PROP_TYPE(BulkTypeTag, CouplingManager);
    auto couplingManager = std::make_shared<CouplingManager>(bulkFvGridGeometry, lowDimFvGridGeometry);

    // the problem (initial and boundary conditions)
    using BulkProblem = typename GET_PROP_TYPE(BulkTypeTag, Problem);
    auto bulkSpatialParams = std::make_shared<typename BulkProblem::SpatialParams>(bulkFvGridGeometry, "Matrix");
    auto bulkProblem = std::make_shared<BulkProblem>(bulkFvGridGeometry, bulkSpatialParams, couplingManager, "Matrix");
    using LowDimProblem = typename GET_PROP_TYPE(LowDimTypeTag, Problem);
    auto lowDimSpatialParams = std::make_shared<typename LowDimProblem::SpatialParams>(lowDimFvGridGeometry, "Fracture");
    auto lowDimProblem = std::make_shared<LowDimProblem>(lowDimFvGridGeometry, lowDimSpatialParams, couplingManager, "Fracture");

    // the solution vector
    Traits::SolutionVector sol;
    sol[bulkIdx].resize(bulkFvGridGeometry->numDofs());
    sol[lowDimIdx].resize(lowDimFvGridGeometry->numDofs());
    bulkProblem->applyInitialSolution(sol[bulkIdx]);
    lowDimProblem->applyInitialSolution(sol[lowDimIdx]);
    auto oldSol = sol;

    couplingManager->init(bulkProblem, lowDimProblem, sol);
    bulkProblem->computePointSourceMap();
    lowDimProblem->computePointSourceMap();

    // the grid variables
    using BulkGridVariables = typename GET_PROP_TYPE(BulkTypeTag, GridVariables);
    auto bulkGridVariables = std::make_shared<BulkGridVariables>(bulkProblem, bulkFvGridGeometry);
    bulkGridVariables->init(sol[bulkIdx], oldSol[bulkIdx]);
    using LowDimGridVariables = typename GET_PROP_TYPE(LowDimTypeTag, GridVariables);
    auto lowDimGridVariables = std::make_shared<LowDimGridVariables>(lowDimProblem, lowDimFvGridGeometry);
    lowDimGridVariables->init(sol[lowDimIdx], oldSol[lowDimIdx]);

    // intialize the vtk output module
    VtkOutputModule<BulkTypeTag> bulkVtkWriter(*bulkProblem, *bulkFvGridGeometry, *bulkGridVariables, sol[bulkIdx], bulkProblem->name());
    GET_PROP_TYPE(BulkTypeTag, VtkOutputFields)::init(bulkVtkWriter);
    bulkVtkWriter.write(0.0);

    VtkOutputModule<LowDimTypeTag> lowDimVtkWriter(*lowDimProblem, *lowDimFvGridGeometry, *lowDimGridVariables, sol[lowDimIdx], lowDimProblem->name());
    GET_PROP_TYPE(LowDimTypeTag, VtkOutputFields)::init(lowDimVtkWriter);
    lowDimVtkWriter.write(0.0);

    // the assembler with time loop for instationary problem
    using Assembler = MultiDomainFVAssembler<Traits, CouplingManager, DiffMethod::numeric>;
    auto assembler = std::make_shared<Assembler>(std::make_tuple(bulkProblem, lowDimProblem),
                                                 std::make_tuple(bulkFvGridGeometry, lowDimFvGridGeometry),
                                                 std::make_tuple(bulkGridVariables, lowDimGridVariables),
                                                 couplingManager);

    // the linear solver
    using LinearSolver = BlockDiagILU0BiCGSTABSolver;
    auto linearSolver = std::make_shared<LinearSolver>();

    // assemble & solve & udpate
    const auto solverType = getParam<std::string>("Problem.SolverType", "linear");
    if (solverType == "linear")
        assembleSolveUpdate(assembler, linearSolver, couplingManager, sol, SolverTag::Linear{});
    else if (solverType == "nonlinear")
        assembleSolveUpdate(assembler, linearSolver, couplingManager, sol, SolverTag::Nonlinear{});
    else
        DUNE_THROW(Dune::IOError, "Invalid solver type " << solverType << "specified in 'Problem.SolverType'!");

    // output the source terms
    bulkProblem->computeSourceIntegral(sol[bulkIdx], *bulkGridVariables);
    lowDimProblem->computeSourceIntegral(sol[lowDimIdx], *lowDimGridVariables);

    // write vtk output
    bulkVtkWriter.write(1.0);
    lowDimVtkWriter.write(1.0);

    ////////////////////////////////////////////////////////////
    // finalize, print dumux message to say goodbye
    ////////////////////////////////////////////////////////////

    // print dumux end message
    if (mpiHelper.rank() == 0)
    {
        Parameters::print();
        DumuxMessage::print(/*firstCall=*/false);
    }

    return 0;
} // end main
catch (Dumux::ParameterException &e)
{
    std::cerr << std::endl << e << " ---> Abort!" << std::endl;
    return 1;
}
catch (Dune::DGFException & e)
{
    std::cerr << "DGF exception thrown (" << e <<
                 "). Most likely, the DGF file name is wrong "
                 "or the DGF file is corrupted, "
                 "e.g. missing hash at end of file or wrong number (dimensions) of entries."
                 << " ---> Abort!" << std::endl;
    return 2;
}
catch (Dune::Exception &e)
{
    std::cerr << "Dune reported error: " << e << " ---> Abort!" << std::endl;
    return 3;
}
catch (...)
{
    std::cerr << "Unknown exception thrown! ---> Abort!" << std::endl;
    return 4;
}