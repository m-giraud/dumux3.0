// $Id: test_diffusion.cc 4144 2010-08-24 10:10:47Z bernd $
/*****************************************************************************
 *   Copyright (C) 20010 by Markus Wolff                                     *
 *   Copyright (C) 2007-2008 by Bernd Flemisch                               *
 *   Copyright (C) 2008-2009 by Andreas Lauser                               *
 *   Institute of Hydraulic Engineering                                      *
 *   University of Stuttgart, Germany                                        *
 *   email: <givenname>.<name>@iws.uni-stuttgart.de                          *
 *                                                                           *
 *   This program is free software; you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation; either version 2 of the License, or       *
 *   (at your option) any later version, as long as this copyright notice    *
 *   is included in its original form.                                       *
 *                                                                           *
 *   This program is distributed WITHOUT ANY WARRANTY.                       *
 *****************************************************************************/
/*!
 * \file
 *
 * \ingroup IMPETtests
 * \brief test for the decoupled one-phase model.
 */
#include "config.h"
#include <iostream>
#include <boost/format.hpp>

#include <dune/common/exceptions.hh>
#include <dune/common/mpihelper.hh>
#include <dune/grid/common/gridinfo.hh>

#include "test_1p_problem.hh"
#include "benchmarkresult.hh"

////////////////////////
// the main function
////////////////////////
void usage(const char *progname)
{
    std::cout << boost::format("usage: %s #refine [delta]\n")%progname;
    exit(1);
}

int main(int argc, char** argv)
{
    try {
        typedef TTAG(TestProblemOneP) TypeTag;
        typedef GET_PROP_TYPE(TypeTag, PTAG(Scalar)) Scalar;
        typedef GET_PROP_TYPE(TypeTag, PTAG(Grid)) Grid;
        static const int dim = Grid::dimension;
        typedef Dune::FieldVector<Scalar, dim> GlobalPosition;

        // initialize MPI, finalize is done automatically on exit
        Dune::MPIHelper::instance(argc, argv);

        ////////////////////////////////////////////////////////////
        // parse the command line arguments
        ////////////////////////////////////////////////////////////
        if (argc != 2 && argc != 3)
            usage(argv[0]);

        int numRefine;
        std::istringstream(argv[1]) >> numRefine;

        double delta = 1e-3;
        if (argc == 3)
            std::istringstream(argv[2]) >> delta;

        ////////////////////////////////////////////////////////////
        // create the grid
        ////////////////////////////////////////////////////////////
        Dune::FieldVector<int,dim> N(1);
        GlobalPosition L(0.0);
        GlobalPosition H(1.0);
        Grid grid(N,L,H);
        grid.globalRefine(numRefine);

        ////////////////////////////////////////////////////////////
        // instantiate and run the concrete problem
        ////////////////////////////////////////////////////////////
//        Dune::Timer timer;
//        bool consecutiveNumbering = true;

        typedef GET_PROP_TYPE(TypeTag, PTAG(Problem)) Problem;
        Problem problem(grid.leafView(), delta);
//        timer.reset();
        problem.init();
        problem.writeOutput();
//        double time = timer.elapsed();
//        Dumux::ResultEvaluation result;
//        result.evaluate(grid.leafView(), problem, problem.variables().pressure(), problem.variables().velocity(), consecutiveNumbering);
//
//        std::cout.setf(std::ios_base::scientific, std::ios_base::floatfield);
//        std::cout.precision(2);
//        std::cout << "\t error press \t error grad\t sumflux\t erflm\t\t uMin\t\t uMax\t\t time" << std::endl;
//        std::cout << "2pfa\t " << result.relativeL2Error << "\t " << result.ergrad << "\t " << result.sumflux
//                        << "\t " << result.erflm << "\t " << result.uMin << "\t " << result.uMax << "\t " << time << std::endl;

        return 0;
    }
    catch (Dune::Exception &e) {
        std::cerr << "Dune reported error: " << e << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown exception thrown!\n";
        throw;
    }

    return 3;
}
