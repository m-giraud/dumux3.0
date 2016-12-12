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
 * \brief Base class for the finite volume geometry vector for mpfa models
 *        This builds up the sub control volumes and sub control volume faces
 *        for each element.
 */
#ifndef DUMUX_DISCRETIZATION_CC_MPFA_GLOBALFVGEOMETRY_BASE_HH
#define DUMUX_DISCRETIZATION_CC_MPFA_GLOBALFVGEOMETRY_BASE_HH

#include <dune/geometry/multilineargeometry.hh>
#include <dune/geometry/referenceelements.hh>

#include <dumux/implicit/cellcentered/mpfa/properties.hh>
#include <dumux/common/elementmap.hh>

namespace Dumux
{
/*!
 * \ingroup ImplicitModel
 * \brief Base class for the finite volume geometry vector for mpfa models
 *        This builds up the sub control volumes and sub control volume faces
 *        for each element.
 */
template<class TypeTag, bool EnableFVElementGeometryCache>
class CCMpfaGlobalFVGeometryBase
{};

// specialization in case the FVElementGeometries are stored
template<class TypeTag>
class CCMpfaGlobalFVGeometryBase<TypeTag, true>
{
    using Implementation = typename GET_PROP_TYPE(TypeTag, GlobalFVGeometry);

    //! The actual implementation might overwrite the update() routine
    friend Implementation;
    //! The local class needs access to the problem
    friend typename GET_PROP_TYPE(TypeTag, FVElementGeometry);

    using Problem = typename GET_PROP_TYPE(TypeTag, Problem);
    using GridView = typename GET_PROP_TYPE(TypeTag, GridView);
    using MpfaHelper = typename GET_PROP_TYPE(TypeTag, MpfaHelper);
    using SubControlVolume = typename GET_PROP_TYPE(TypeTag, SubControlVolume);
    using SubControlVolumeFace = typename GET_PROP_TYPE(TypeTag, SubControlVolumeFace);
    using FVElementGeometry = typename GET_PROP_TYPE(TypeTag, FVElementGeometry);
    using GlobalInteractionVolumeSeeds = typename GET_PROP_TYPE(TypeTag, GlobalInteractionVolumeSeeds);
    using BoundaryInteractionVolume = typename GET_PROP_TYPE(TypeTag, BoundaryInteractionVolume);
    using InteractionVolume = typename GET_PROP_TYPE(TypeTag, InteractionVolume);
    using InteractionVolumeSeed = typename InteractionVolume::Seed;
    using BoundaryInteractionVolumeSeed = typename BoundaryInteractionVolume::Seed;
    using Element = typename GridView::template Codim<0>::Entity;

    using Scalar = typename GET_PROP_TYPE(TypeTag, Scalar);
    using CoordScalar = typename GridView::ctype;
    using IndexType = typename GridView::IndexSet::IndexType;
    using LocalIndexType = typename InteractionVolume::LocalIndexType;

    static const int dim = GridView::dimension;
    static const int dimWorld = GridView::dimensionworld;

    using GlobalPosition = Dune::FieldVector<Scalar, dimWorld>;
    using ReferenceElements = typename Dune::ReferenceElements<CoordScalar, dim>;

public:
    //! Constructor
    CCMpfaGlobalFVGeometryBase(const GridView gridView)
    : gridView_(gridView), elementMap_(gridView), globalInteractionVolumeSeeds_(gridView) {}

    //! The total number of sub control volumes
    std::size_t numScv() const
    { return scvs_.size(); }

    //! The total number of sun control volume faces
    std::size_t numScvf() const
    { return scvfs_.size(); }

    //! The total number of boundary sub control volume faces
    std::size_t numBoundaryScvf() const
    { return numBoundaryScvf_; }

    // Get an element from a sub control volume contained in it
    Element element(const SubControlVolume& scv) const
    { return elementMap_.element(scv.elementIndex()); }

    // Get an element from a global element index
    Element element(IndexType eIdx) const
    { return elementMap_.element(eIdx); }

    //! Get the inner interaction volume seed corresponding to an scvf
    const InteractionVolumeSeed& interactionVolumeSeed(const SubControlVolumeFace& scvf) const
    { return globalInteractionVolumeSeeds_.seed(scvf); }

    //! Get the boundary interaction volume seed corresponding to an scvf
    const BoundaryInteractionVolumeSeed& boundaryInteractionVolumeSeed(const SubControlVolumeFace& scvf) const
    { return globalInteractionVolumeSeeds_.boundarySeed(scvf); }

    //! Returns whether or not an scvf touches the boundary (has to be called before getting an interaction volume)
    bool scvfTouchesBoundary(const SubControlVolumeFace& scvf) const
    { return boundaryVertices_[scvf.vertexIndex()]; }

    //! Returns whether or not an scvf touches a branching point (for dim < dimWorld)
    bool scvfTouchesBranchingPoint(const SubControlVolumeFace& scvf) const
    {
        if (dim == dimWorld)
            return false;
        else
            return branchingVertices_[scvf.vertexIndex()];
    }

    //! update all fvElementGeometries (do this again after grid adaption)
    void update(const Problem& problem)
    {
        problemPtr_ = &problem;

        // clear containers (necessary after grid refinement)
        scvs_.clear();
        scvfs_.clear();
        scvfIndicesOfScv_.clear();
        boundaryVertices_.clear();
        elementMap_.clear();

        // reserve memory
        std::size_t numScvs = gridView_.size(0);
        std::size_t numScvfs = MpfaHelper::getGlobalNumScvf(gridView_);
        scvs_.resize(numScvs);
        scvfs_.reserve(numScvfs);
        scvfIndicesOfScv_.resize(numScvs);
        boundaryVertices_.resize(gridView_.size(dim), false);
        elementMap_.resize(numScvs);

        // keep track of branching points
        if (dim < dimWorld)
            branchingVertices_.resize(gridView_.size(dim), false);

        // find vertices on processor boundaries
        auto isGhostVertex = MpfaHelper::findGhostVertices(problem, gridView_);

        // the quadrature point to be used on the scvf
        const Scalar q = GET_PARAM_FROM_GROUP(TypeTag, Scalar, Mpfa, Q);

        // Instantiate a helper class to pass it to the scvf constructors
        MpfaHelper helper;

        // Build the SCVs and SCV faces
        IndexType scvfIdx = 0;
        numBoundaryScvf_ = 0;
        for (const auto& element : elements(gridView_))
        {
            auto eIdx = problem.elementMapper().index(element);

            // the element geometry
            auto elementGeometry = element.geometry();

            // fill the element map with seeds
            elementMap_[eIdx] = element.seed();

            // The local scvf index set
            std::vector<IndexType> scvfIndexSet;
            scvfIndexSet.reserve(MpfaHelper::getNumLocalScvfs(elementGeometry.type()));

            // for network grids there might be multiple intersection with the same geometryInInside
            // we indentify those by the indexInInside for now (assumes conforming grids at branching facets)
            std::vector<std::vector<IndexType>> outsideIndices;
            if (dim < dimWorld)
            {
                outsideIndices.resize(element.subEntities(1));
                for (const auto& intersection : intersections(gridView_, element))
                {
                    if (intersection.neighbor())
                    {
                        const auto& outside = intersection.outside();
                        auto nIdx = problem.elementMapper().index(outside);
                        outsideIndices[intersection.indexInInside()].push_back(nIdx);
                    }
                }
            }

            // construct the sub control volume faces
            for (const auto& is : intersections(gridView_, element))
            {
                auto indexInInside = is.indexInInside();
                bool boundary = is.boundary();
                bool neighbor = is.neighbor();

                // for network grids, skip the rest if handled already
                if (dim < dimWorld && neighbor && outsideIndices[indexInInside].empty())
                    continue;

                // determine the outside volvar indices
                std::vector<IndexType> nIndices;
                if (neighbor && dim == dimWorld)
                    nIndices = std::vector<IndexType>( {problem.elementMapper().index(is.outside())} );

                // get the intersection corners according to generic numbering
                auto numCorners = is.geometry().corners();
                std::vector<GlobalPosition> isCorners(numCorners);

                // if outside level > inside level, use the outside element in the following
                bool useNeighbor = neighbor && is.outside().level() > element.level();
                const auto& e = useNeighbor ? is.outside() : element;
                const auto indexInElement = useNeighbor ? is.indexInOutside() : indexInInside;
                const auto eg = e.geometry();
                const auto& refElement = ReferenceElements::general(eg.type());

                for (unsigned int c = 0; c < numCorners; ++c)
                    isCorners[c] = eg.global(refElement.position(refElement.subEntity(indexInElement, 1, c, dim), dim));

                // make the scv faces belonging to each corner of the intersection
                for (unsigned int c = 0; c < numCorners; ++c)
                {
                    // get the global vertex index the scv face is connected to
                    auto vIdxLocal = refElement.subEntity(indexInElement, 1, c, dim);
                    auto vIdxGlobal = problem.vertexMapper().subIndex(e, vIdxLocal, dim);

                    // do not build scvfs connected to a processor boundary
                    if (isGhostVertex[vIdxGlobal])
                        continue;

                    // store info on which vertices are on the domain boundary
                    if (boundary)
                        boundaryVertices_[vIdxGlobal] = true;

                    // is vertex on a branching point?
                    if (dim < dimWorld && outsideIndices[indexInInside].size() > 1)
                        branchingVertices_[vIdxGlobal] = true;

                    // make the scv face (for inside scvfs on network grids, use precalculated outside indices)
                    if (!boundary)
                    {
                        const auto& outsideScvIndices = dim == dimWorld ? nIndices : outsideIndices[indexInInside];
                        scvfIndexSet.push_back(scvfIdx);
                        scvfs_.emplace_back(helper,
                                            helper.getScvfCorners(isCorners, c),
                                            is.centerUnitOuterNormal(),
                                            vIdxGlobal,
                                            vIdxLocal,
                                            scvfIdx,
                                            eIdx,
                                            outsideScvIndices,
                                            q,
                                            boundary
                                            );
                    }
                    else
                    {
                        nIndices.resize(1);
                        nIndices[0] = numScvs + numBoundaryScvf_++;
                        scvfIndexSet.push_back(scvfIdx);
                        scvfs_.emplace_back(helper,
                                            helper.getScvfCorners(isCorners, c),
                                            is.centerUnitOuterNormal(),
                                            vIdxGlobal,
                                            vIdxLocal,
                                            scvfIdx,
                                            eIdx,
                                            nIndices,
                                            q,
                                            boundary
                                            );
                    }

                    // increment scvf counter
                    scvfIdx++;
                }

                // for network grids, clear outside indices to not make a second scvf on that facet
                if (dim < dimWorld)
                    outsideIndices[indexInInside].clear();
            }

            // create sub control volume for this element
            scvs_[eIdx] = SubControlVolume(std::move(elementGeometry), eIdx);

            // Save the scvf indices belonging to this scv to build up fv element geometries fast
            scvfIndicesOfScv_[eIdx] = scvfIndexSet;
        }

        // the number of actual boundary scvf is two times the number of boundary intersections
        numBoundaryScvf_ *= 2;

        // in parallel problems we might have reserved more scvfs than we actually use
        scvfs_.shrink_to_fit();

        // Make the flip index set for network and surface grids
        if (dim < dimWorld)
        {
            flipScvfIndices_.resize(scvfs_.size());
            for (auto&& scvf : scvfs_)
            {
                if (scvf.boundary())
                    continue;

                const auto numOutsideScvs = scvf.numOutsideScvs();
                const auto vIdxGlobal = scvf.vertexIndex();
                const auto insideScvIdx = scvf.insideScvIdx();

                flipScvfIndices_[scvf.index()].resize(numOutsideScvs);
                for (unsigned int i = 0; i < numOutsideScvs; ++i)
                {
                    const auto outsideScvIdx = scvf.outsideScvIdx(i);
                    for (auto outsideScvfIndex : scvfIndicesOfScv_[outsideScvIdx])
                    {
                        const auto& outsideScvf = this->scvf(outsideScvfIndex);
                        if (outsideScvf.vertexIndex() == vIdxGlobal &&
                            MpfaHelper::contains(outsideScvf.outsideScvIndices(), insideScvIdx))
                        {
                            flipScvfIndices_[scvf.index()][i] = outsideScvfIndex;
                            // there is always only one flip face in an outside element
                            break;
                        }
                    }

                }
            }
        }

        // Initialize the interaction volume seeds
        globalInteractionVolumeSeeds_.update(problem, boundaryVertices_);
    }

    /*!
     * \brief Return a local restriction of this global object
     *        The local object is only functional after calling its bind/bindElement method
     *        This is a free function that will be found by means of ADL
     */
    friend inline FVElementGeometry localView(const Implementation& global)
    { return FVElementGeometry(global); }

    //! Get a sub control volume with a global scv index
    const SubControlVolume& scv(IndexType scvIdx) const
    { return scvs_[scvIdx]; }

    //! Get a sub control volume face with a global scvf index
    const SubControlVolumeFace& scvf(IndexType scvfIdx) const
    { return scvfs_[scvfIdx]; }

    const SubControlVolumeFace& flipScvf(IndexType scvfIdx, unsigned int outsideScvfIdx = 0) const
    {
        return scvfs_[flipScvfIndices_[scvfIdx][outsideScvfIdx]];
    }

    //! Get the sub control volume face indices of an scv by global index
    const std::vector<IndexType>& scvfIndicesOfScv(IndexType scvIdx) const
    { return scvfIndicesOfScv_[scvIdx]; }

    //! Return a const reference to the grid view
    const GridView& gridView() const
    { return gridView_; }

private:

    const Problem& problem_() const
    { return *problemPtr_; }

    const Problem* problemPtr_;
    GridView gridView_;

    // vectors that store the geometries
    Dumux::ElementMap<GridView> elementMap_;
    std::vector<SubControlVolume> scvs_;
    std::vector<SubControlVolumeFace> scvfs_;

    // vectors that store the global data
    std::vector<std::vector<IndexType>> scvfIndicesOfScv_;
    std::vector<bool> boundaryVertices_;
    std::vector<bool> branchingVertices_;
    IndexType numBoundaryScvf_;
    // needed for embedded surface and network grids (dim < dimWorld)
    std::vector<std::vector<IndexType>> flipScvfIndices_;

    // the global interaction volume seeds
    GlobalInteractionVolumeSeeds globalInteractionVolumeSeeds_;
};

// specialization in case the FVElementGeometries are not stored
template<class TypeTag>
class CCMpfaGlobalFVGeometryBase<TypeTag, false>
{
    //! The local fvGeometry needs access to the problem
    friend typename GET_PROP_TYPE(TypeTag, FVElementGeometry);

    using Implementation = typename GET_PROP_TYPE(TypeTag, GlobalFVGeometry);
    using Problem = typename GET_PROP_TYPE(TypeTag, Problem);
    using GridView = typename GET_PROP_TYPE(TypeTag, GridView);
    using MpfaHelper = typename GET_PROP_TYPE(TypeTag, MpfaHelper);
    using SubControlVolume = typename GET_PROP_TYPE(TypeTag, SubControlVolume);
    using SubControlVolumeFace = typename GET_PROP_TYPE(TypeTag, SubControlVolumeFace);
    using FVElementGeometry = typename GET_PROP_TYPE(TypeTag, FVElementGeometry);
    using GlobalInteractionVolumeSeeds = typename GET_PROP_TYPE(TypeTag, GlobalInteractionVolumeSeeds);
    using BoundaryInteractionVolume = typename GET_PROP_TYPE(TypeTag, BoundaryInteractionVolume);
    using InteractionVolume = typename GET_PROP_TYPE(TypeTag, InteractionVolume);
    using InteractionVolumeSeed = typename InteractionVolume::Seed;
    using BoundaryInteractionVolumeSeed = typename BoundaryInteractionVolume::Seed;
    using Element = typename GridView::template Codim<0>::Entity;

    using Scalar = typename GET_PROP_TYPE(TypeTag, Scalar);
    using CoordScalar = typename GridView::ctype;
    using IndexType = typename GridView::IndexSet::IndexType;
    using LocalIndexType = typename InteractionVolume::LocalIndexType;

    static const int dim = GridView::dimension;
    static const int dimWorld = GridView::dimensionworld;

    using GlobalPosition = Dune::FieldVector<Scalar, dimWorld>;
    using ReferenceElements = typename Dune::ReferenceElements<CoordScalar, dim>;

public:
    //! Constructor
    CCMpfaGlobalFVGeometryBase(const GridView gridView)
    : gridView_(gridView), elementMap_(gridView), globalInteractionVolumeSeeds_(gridView_) {}

    //! The total number of sub control volumes
    std::size_t numScv() const
    { return numScvs_; }

    //! The total number of sub control volume faces
    std::size_t numScvf() const
    { return numScvf_; }

    //! The total number of boundary sub control volume faces
    std::size_t numBoundaryScvf() const
    { return numBoundaryScvf_; }

    // Get an element from a sub control volume contained in it
    Element element(const SubControlVolume& scv) const
    { return elementMap_.element(scv.elementIndex()); }

    // Get an element from a global element index
    Element element(IndexType eIdx) const
    { return elementMap_.element(eIdx); }

    //! Return the gridView this global object lives on
    const GridView& gridView() const
    { return gridView_; }

    //! Get the inner interaction volume seed corresponding to an scvf
    const InteractionVolumeSeed& interactionVolumeSeed(const SubControlVolumeFace& scvf) const
    { return globalInteractionVolumeSeeds_.seed(scvf); }

    //! Get the boundary interaction volume seed corresponding to an scvf
    const BoundaryInteractionVolumeSeed& boundaryInteractionVolumeSeed(const SubControlVolumeFace& scvf) const
    { return globalInteractionVolumeSeeds_.boundarySeed(scvf); }

    //! Returns whether or not an scvf touches the boundary (has to be called before getting an interaction volume)
    bool scvfTouchesBoundary(const SubControlVolumeFace& scvf) const
    { return boundaryVertices_[scvf.vertexIndex()]; }

    //! Returns whether or not a vertex is on a processor boundary
    bool isGhostVertex(const IndexType vIdxGlobal) const
    { return ghostVertices_[vIdxGlobal]; }

    //! Returns whether or not an scvf touches a branching point (for dim < dimWorld)
    bool scvfTouchesBranchingPoint(const SubControlVolumeFace& scvf) const
    { return branchingVertices_[scvf.vertexIndex()]; }

    //! update all fvElementGeometries (do this again after grid adaption)
    void update(const Problem& problem)
    {
        problemPtr_ = &problem;

        // clear containers (necessary after grid refinement)
        scvfIndicesOfScv_.clear();
        neighborVolVarIndices_.clear();
        boundaryVertices_.clear();
        elementMap_.clear();

        // reserve memory or resize the containers
        numScvs_ = gridView_.size(0);
        numScvf_ = 0;
        numBoundaryScvf_ = 0;
        elementMap_.resize(numScvs_);
        scvfIndicesOfScv_.resize(numScvs_);
        neighborVolVarIndices_.resize(numScvs_);
        boundaryVertices_.resize(gridView_.size(dim), false);

        // keep track of branching points
        if (dim < dimWorld)
            branchingVertices_.resize(gridView_.size(dim), false);

        // find vertices on processor boundaries
        ghostVertices_ = MpfaHelper::findGhostVertices(problem, gridView_);

        // Store necessary info on SCVs and SCV faces
        for (const auto& element : elements(gridView_))
        {
            auto eIdx = problem.elementMapper().index(element);

            // fill the element map with seeds
            elementMap_[eIdx] = element.seed();

            // the element geometry and reference element
            auto eg = element.geometry();

            // the element-wise index sets for finite volume geometry
            auto numLocalFaces = MpfaHelper::getNumLocalScvfs(eg.type());
            std::vector<IndexType> scvfsIndexSet;
            std::vector< std::vector<IndexType> > neighborVolVarIndexSet;
            scvfsIndexSet.reserve(numLocalFaces);
            neighborVolVarIndexSet.reserve(numLocalFaces);

            // for network grids there might be multiple intersection with the same geometryInInside
            // we indentify those by the indexInInside for now (assumes conforming grids at branching facets)
            std::vector<std::vector<IndexType>> outsideIndices;
            if (dim < dimWorld)
            {
                outsideIndices.resize(element.subEntities(1));
                for (const auto& intersection : intersections(gridView_, element))
                {
                    if (intersection.neighbor())
                    {
                        const auto& outside = intersection.outside();
                        auto nIdx = problem.elementMapper().index(outside);
                        outsideIndices[intersection.indexInInside()].push_back(nIdx);
                    }
                }
            }

            unsigned int localFaceIdx = 0;
            // construct the sub control volume faces
            for (const auto& is : intersections(gridView_, element))
            {
                auto indexInInside = is.indexInInside();
                bool boundary = is.boundary();
                bool neighbor = is.neighbor();
                // for network grids, skip the rest if handled already
                if (dim < dimWorld && neighbor && outsideIndices[indexInInside].empty())
                    continue;

                // determine the outside volvar indices
                std::vector<IndexType> nIndices;
                if (neighbor && dim == dimWorld)
                    nIndices = std::vector<IndexType>( {problem.elementMapper().index(is.outside())} );

                // if outside level > inside level, use the outside element in the following
                bool useNeighbor = neighbor && is.outside().level() > element.level();
                const auto& e = useNeighbor ? is.outside() : element;
                const auto indexInElement = useNeighbor ? is.indexInOutside() : indexInInside;
                const auto eg = e.geometry();
                const auto& refElement = ReferenceElements::general(eg.type());

                // make the scv faces of the intersection
                for (unsigned int c = 0; c < is.geometry().corners(); ++c)
                {
                    // get the global vertex index the scv face is connected to (mpfa-o method does not work for hanging nodes!)
                    const auto vIdxLocal = refElement.subEntity(indexInElement, 1, c, dim);
                    const auto vIdxGlobal = problem.vertexMapper().subIndex(e, vIdxLocal, dim);

                    // do not build scvfs connected to a processor boundary
                    if (ghostVertices_[vIdxGlobal])
                        continue;

                    // store info on which vertices are on the domain boundary
                    if (boundary)
                        boundaryVertices_[vIdxGlobal] = true;

                    // is vertex on a branching point?
                    if (dim < dimWorld && outsideIndices[indexInInside].size() > 1)
                        branchingVertices_[vIdxGlobal] = true;

                    // store information on the scv face (for inner scvfs on network grids use precalculated outside indices)
                    if (!boundary)
                    {
                        const auto& outsideScvIndices = dim == dimWorld ? nIndices : outsideIndices[indexInInside];
                        scvfsIndexSet.push_back(numScvf_++);
                        neighborVolVarIndexSet.push_back(outsideScvIndices);
                    }
                    else
                    {
                        nIndices.resize(1);
                        nIndices[0] = numScvs_ + numBoundaryScvf_++;
                        scvfsIndexSet.push_back(numScvf_++);
                        neighborVolVarIndexSet.push_back(nIndices);
                    }

                    // increment counter
                    localFaceIdx++;
                }

                // for network grids, clear outside indices to not make a second scvf on that facet
                if (dim < dimWorld)
                    outsideIndices[indexInInside].clear();
            }

            // store the sets of indices in the data container
            scvfIndicesOfScv_[eIdx] = scvfsIndexSet;
            neighborVolVarIndices_[eIdx] = neighborVolVarIndexSet;
        }

        // the number of actual boundary scvf is two times the number of boundary intersections
        numBoundaryScvf_ *= 2;

        // Initialize the interaction volume seeds
        globalInteractionVolumeSeeds_.update(problem, boundaryVertices_);
    }

    const std::vector<IndexType>& scvfIndicesOfScv(IndexType scvIdx) const
    { return scvfIndicesOfScv_[scvIdx]; }

    const std::vector< std::vector<IndexType> >& neighborVolVarIndices(IndexType scvIdx) const
    { return neighborVolVarIndices_[scvIdx]; }

    /*!
     * \brief Return a local restriction of this global object
     *        The local object is only functional after calling its bind/bindElement method
     *        This is a free function that will be found by means of ADL
     */
    friend inline FVElementGeometry localView(const Implementation& global)
    { return FVElementGeometry(global); }

private:

    const Problem& problem_() const
    { return *problemPtr_; }

    const Problem* problemPtr_;

    GridView gridView_;

    // Information on the global number of geometries
    IndexType numScvs_;
    IndexType numScvf_;
    IndexType numBoundaryScvf_;

    // vectors that store the global data
    Dumux::ElementMap<GridView> elementMap_;
    std::vector<std::vector<IndexType>> scvfIndicesOfScv_;
    std::vector< std::vector< std::vector<IndexType> > > neighborVolVarIndices_;
    std::vector<bool> boundaryVertices_;
    std::vector<bool> ghostVertices_;
    std::vector<bool> branchingVertices_;

    // the global interaction volume seeds
    GlobalInteractionVolumeSeeds globalInteractionVolumeSeeds_;
};

} // end namespace

#endif
