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
 * \brief Base class for all models which use the one-phase,
 *        fully implicit model.
 *        Adaption of the fully implicit scheme to the one-phase flow model.
 */

#ifndef DUMUX_1P_MODEL_HH
#define DUMUX_1P_MODEL_HH

// #include <dumux/porousmediumflow/implicit/velocityoutput.hh>
#include "properties.hh"

namespace Dumux
{
/*!
 * \ingroup NavierStokesModel
 * \brief A single-phase, isothermal flow model using the fully implicit scheme.
 *
 * Single-phase, isothermal flow model, which uses a standard Darcy approach as the
 * equation for the conservation of momentum:
 * \f[
 v = - \frac{\textbf K}{\mu}
 \left(\textbf{grad}\, p - \varrho {\textbf g} \right)
 * \f]
 *
 * and solves the mass continuity equation:
 * \f[
 \phi \frac{\partial \varrho}{\partial t} + \text{div} \left\lbrace
 - \varrho \frac{\textbf K}{\mu} \left( \textbf{grad}\, p -\varrho {\textbf g} \right) \right\rbrace = q,
 * \f]
 * All equations are discretized using a vertex-centered finite volume (box)
 * or cell-centered finite volume scheme as spatial
 * and the implicit Euler method as time discretization.
 * The model supports compressible as well as incompressible fluids.
 */
template<class TypeTag >
class NavierStokesModel : public GET_PROP_TYPE(TypeTag, BaseModel)
{
    typedef typename GET_PROP_TYPE(TypeTag, FVElementGeometry) FVElementGeometry;
//     typedef typename GET_PROP_TYPE(TypeTag, SpatialParams) SpatialParams;
    typedef typename GET_PROP_TYPE(TypeTag, SolutionVector) SolutionVector;

    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;
    enum { dim = GridView::dimension };
    enum { dimWorld = GridView::dimensionworld };

    enum { isBox = GET_PROP_VALUE(TypeTag, ImplicitIsBox) };
    enum { dofCodim = isBox ? dim : 0 };
    using StencilsVector = typename GET_PROP_TYPE(TypeTag, StencilsVector);
    using Element = typename GridView::template Codim<0>::Entity;

public:
    /*!
     * \brief \copybrief Dumux::ImplicitModel::addOutputVtkFields
     *
     * Specialization for the NavierStokesModel, adding the pressure and
     * the process rank to the VTK writer.
     */
    template<class MultiWriter>
    void addOutputVtkFields(const SolutionVector &sol,
                            MultiWriter &writer)
    {
        // typedef Dune::BlockVector<Dune::FieldVector<double, dimWorld> > VectorField;

        // create the required scalar fields
        unsigned numDofs = this->numDofs();
        auto *p = writer.allocateManagedBuffer(numDofs);
        // VectorField *velocity = writer.template allocateManagedBuffer<double, dimWorld>(numDofs);
        // ImplicitVelocityOutput<TypeTag> velocityOutput(this->problem_());

        // if (velocityOutput.enableOutput())
        // {
        //     // initialize velocity field
        //     for (unsigned int i = 0; i < numDofs; ++i)
        //     {
        //         (*velocity)[i] = double(0);
        //     }
        // }

        unsigned numElements = this->gridView_().size(0);
        auto *rank = writer.allocateManagedBuffer(numElements);

        for (const auto& element : elements(this->gridView_(), Dune::Partitions::interior))
        {
            auto eIdx = this->elementMapper().index(element);
            (*rank)[eIdx] = this->gridView_().comm().rank();

            // get the local fv geometry
            auto fvGeometry = localView(this->globalFvGeometry());
            fvGeometry.bindElement(element);

            auto elemVolVars = localView(this->curGlobalVolVars());
            elemVolVars.bindElement(element, fvGeometry, this->curSol());

            for (auto&& scv : scvs(fvGeometry))
            {
                const auto& volVars = elemVolVars[scv];
                auto dofIdxGlobal = scv.dofIndex();

                (*p)[dofIdxGlobal] = volVars.pressure();
            }

            // velocity output
            //velocityOutput.calculateVelocity(*velocity, elemVolVars, fvGeometry, element, /*phaseIdx=*/0);
        }

        writer.attachDofData(*p, "p", isBox);
        // if (velocityOutput.enableOutput())
        // {
        //     writer.attachDofData(*velocity,  "velocity", isBox, dim);
        // }
        writer.attachCellData(*rank, "process rank");
    }

     /*!
     * \brief Returns the number of global degrees of freedoms (DOFs)
     */
    size_t numDofs() const
    {
        return this->gridView_().size(0) + this->gridView_().size(1);
    }

     /*!
     * \brief Returns the number of cell center degrees of freedoms (DOFs)
     */
    size_t numCellCenterDofs() const
    {
        return this->gridView_().size(0);
    }

     /*!
     * \brief Returns the number of cell center degrees of freedoms (DOFs)
     */
    size_t numFaceDofs() const
    {
        return this->gridView_().size(1);
    }

    /*!
    * \brief Returns the size of a complete face dof stencil
    */
    size_t fullFaceToCellCenterStencilSize(const int idx) const
    {
        return this->stencilsVector_.fullFaceToCellCenterStencilSize(idx);
    }

    /*!
    * \brief Returns the size of a complete face dof stencil
    */
    size_t fullfaceToFaceStencilSize(const int idx) const
    {
        return this->stencilsVector_.fullfaceToFaceStencilSize(idx);
    }

    /*!
    * \brief Returns a unique pointer to the complete face dof stencils which is used once for setting up the global matrix and deleted afterwards
    */
    auto getFullFaceToCellCenterStencilsPtr()
    {
        return this->stencilsVector_.getFullFaceToCellCenterStencilsPtr();
    }

    /*!
    * \brief Returns a unique pointer to the complete face dof stencils which is used once for setting up the global matrix and deleted afterwards
    */
    auto getFullfaceToFaceStencilsPtr()
    {
        return this->stencilsVector_.getFullfaceToFaceStencilsPtr();
    }

};
}

#include "propertydefaults.hh"

#endif
