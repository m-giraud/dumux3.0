/*****************************************************************************
 *   Copyright (C) 2011 by Yufei Cao                                         *
 *   Institute of Hydraulic Engineering                                      *
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
 *
 * \ingroup IMPETtests
 * \brief test for diffusion models
 */
#include "config.h"
#include <iostream>
#include <boost/format.hpp>

#include <dune/common/exceptions.hh>
#include <dune/common/version.hh>
#if DUNE_VERSION_NEWER(DUNE_COMMON, 2, 3)
#include <dune/common/parallel/mpihelper.hh>
#else
#include <dune/common/mpihelper.hh>
#endif

#include <dune/grid/common/gridinfo.hh>
#include <dune/common/parametertreeparser.hh>

#include "test_diffusionproblem3d.hh"
#include "resultevaluation3d.hh"

////////////////////////
// the main function
////////////////////////
void usage(const char *progname)
{
    std::cout << boost::format("usage: %s input-file-name\n") % progname;
    exit(1);
}

int main(int argc, char** argv)
{
    try {
        typedef TTAG(DiffusionTestProblem) TypeTag;
        typedef GET_PROP_TYPE(TypeTag, Grid) Grid;
        typedef typename GET_PROP(TypeTag, ParameterTree) ParameterTree;

        // initialize MPI, finalize is done automatically on exit
        Dune::MPIHelper::instance(argc, argv);

        ////////////////////////////////////////////////////////////
        // parse the command line arguments
        ////////////////////////////////////////////////////////////
        if (argc > 2)
            usage(argv[0]);

        //read inputfile name
        std::string inputFileName("test_diffusion3d.input");
        if (argc == 2)
            inputFileName = argv[1];

        Dune::ParameterTreeParser::readINITree(inputFileName, ParameterTree::tree());
//
        int numRefine = 0;
        if (ParameterTree::tree().hasKey("Grid.RefinementRatio"))
        numRefine = GET_RUNTIME_PARAM_FROM_GROUP(TypeTag, int, Grid, RefinementRatio);

        std::string outputName("");
        if (ParameterTree::tree().hasKey("Problem.OutputName"))
        {
            outputName += "_";
            outputName += GET_RUNTIME_PARAM(TypeTag, std::string, OutputName);
        }

        ////////////////////////////////////////////////////////////
        // create the grid
        ////////////////////////////////////////////////////////////
        std::string gridFileName = GET_RUNTIME_PARAM_FROM_GROUP(TypeTag, std::string, Grid, File);
        Dune::GridPtr < Grid > grid(gridFileName);
        grid->globalRefine(numRefine);

        Dune::gridinfo (*grid);

        ////////////////////////////////////////////////////////////
        // instantiate and run the concrete problem
        ////////////////////////////////////////////////////////////
        Dune::Timer timer;
        bool consecutiveNumbering = true;

        typedef TTAG (FVTestProblem)
        FVTypeTag;
        typedef GET_PROP_TYPE(FVTypeTag, Problem) FVProblem;
        typedef typename GET_PROP(FVTypeTag, ParameterTree) FVParameterTree;
        Dune::ParameterTreeParser::readINITree(inputFileName, FVParameterTree::tree());
        FVProblem *fvProblem = new FVProblem(grid->leafView());
        std::string fvOutput("test_diffusion3d_fv");
        fvOutput += outputName;
        if (numRefine > 0)
        {
            char refine[128];
            sprintf(refine, "_numRefine%d", numRefine);
            fvOutput += refine;
        }
        fvProblem->setName(fvOutput.c_str());
        timer.reset();
        fvProblem->init();
        fvProblem->calculateFVVelocity();
        double fvTime = timer.elapsed();
        fvProblem->writeOutput();
        Dumux::ResultEvaluation fvResult;
        fvResult.evaluate(grid->leafView(), *fvProblem, consecutiveNumbering);

        delete fvProblem;

        typedef TTAG (FVMPFAL3DTestProblem)
        MPFALTypeTag;
        typedef GET_PROP_TYPE(MPFALTypeTag, Problem) MPFALProblem;
        typedef typename GET_PROP(MPFALTypeTag, ParameterTree) MPFALParameterTree;
        Dune::ParameterTreeParser::readINITree(inputFileName, MPFALParameterTree::tree());
        MPFALProblem *mpfaProblem = new MPFALProblem(grid->leafView());
        std::string fvmpfaOutput("test_diffusion3d_fvmpfal");
        fvmpfaOutput += outputName;
        if (numRefine > 0)
        {
            char refine[128];
            sprintf(refine, "_numRefine%d", numRefine);
            fvmpfaOutput += refine;
        }
        mpfaProblem->setName(fvmpfaOutput.c_str());
        timer.reset();
        mpfaProblem->init();
        double mpfaTime = timer.elapsed();
        mpfaProblem->writeOutput();
        Dumux::ResultEvaluation mpfaResult;
        mpfaResult.evaluate(grid->leafView(), *mpfaProblem, consecutiveNumbering);

        delete mpfaProblem;

        typedef TTAG (MimeticTestProblem)
        MimeticTypeTag;
        typedef GET_PROP_TYPE(MimeticTypeTag, Problem) MimeticProblem;
        typedef typename GET_PROP(MimeticTypeTag, ParameterTree) MimeticParameterTree;
        Dune::ParameterTreeParser::readINITree(inputFileName, MimeticParameterTree::tree());
        MimeticProblem *mimeticProblem = new MimeticProblem(grid->leafView());
        std::string mimeticOutput("test_diffusion3d_mimetic");
        mimeticOutput += outputName;
        if (numRefine > 0)
        {
            char refine[128];
            sprintf(refine, "_numRefine%d", numRefine);
            mimeticOutput += refine;
        }
        mimeticProblem->setName(mimeticOutput.c_str());
        timer.reset();
        mimeticProblem->init();
        double mimeticTime = timer.elapsed();
        mimeticProblem->writeOutput();
        Dumux::ResultEvaluation mimeticResult;
        mimeticResult.evaluate(grid->leafView(), *mimeticProblem, consecutiveNumbering);

        delete mimeticProblem;

        std::cout.setf(std::ios_base::scientific, std::ios_base::floatfield);
        std::cout.precision(2);
        std::cout
                << "\t pErrorL2 \t pInnerErrorL2 \t vErrorL2 \t vInnerErrorL2 \t hMax \t\t pMin\t\t pMax\t\t pMinExact\t pMaxExact\t time"
                << std::endl;
        std::cout << "2pfa\t " << fvResult.relativeL2Error << "\t " << fvResult.relativeL2ErrorIn << "\t "
                << fvResult.ervell2 << "\t " << fvResult.ervell2In << "\t " << fvResult.hMax << "\t " << fvResult.uMin
                << "\t " << fvResult.uMax << "\t " << fvResult.uMinExact << "\t " << fvResult.uMaxExact << "\t "
                << fvTime << std::endl;
        std::cout << "mpfa-l\t " << mpfaResult.relativeL2Error << "\t " << mpfaResult.relativeL2ErrorIn << "\t "
                << mpfaResult.ervell2 << "\t " << mpfaResult.ervell2In << "\t " << mpfaResult.hMax << "\t "
                << mpfaResult.uMin << "\t " << mpfaResult.uMax << "\t " << mpfaResult.uMinExact << "\t "
                << mpfaResult.uMaxExact << "\t " << mpfaTime << std::endl;
        std::cout << "mimetic\t " << mimeticResult.relativeL2Error << "\t " << mimeticResult.relativeL2ErrorIn << "\t "
                << mimeticResult.ervell2 << "\t " << mimeticResult.ervell2In << "\t " << mimeticResult.hMax << "\t "
                << mimeticResult.uMin << "\t " << mimeticResult.uMax << "\t " << mimeticResult.uMinExact << "\t "
                << mimeticResult.uMaxExact << "\t " << mimeticTime << std::endl;

        return 0;
    } catch (Dune::Exception &e)
    {
        std::cerr << "Dune reported error: " << e << std::endl;
    } catch (...)
    {
        std::cerr << "Unknown exception thrown!\n";
        throw;
    }

    return 3;
}