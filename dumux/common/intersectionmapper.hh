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
#ifndef DUMUX_INTERSECTIONITERATOR_HH
#define DUMUX_INTERSECTIONITERATOR_HH

#include<vector>
#include<unordered_map>
#include<dune/grid/common/mcmgmapper.hh>

/**
 * @file
 * @brief  defines an intersection mapper for mapping of global DOF's assigned to faces which also works for adaptive grids.
 */

namespace Dumux
{

template<class GridView>
class IntersectionMapper
{
    // mapper: one data element in every entity
    template<int dim>
    struct ElementLayout
    {
        bool contains (Dune::GeometryType gt)
        {
            return gt.dim() == dim;
        }
    };

    typedef typename GridView::Grid Grid;
    enum {dim=Grid::dimension};
    typedef typename Grid::template Codim<0>::Entity Element;
    typedef typename Grid::template Codim<0>::EntityPointer ElementPointer;
    typedef typename GridView::template Codim<0>::Iterator ElementIterator;
    typedef typename GridView::IntersectionIterator IntersectionIterator;
    typedef typename GridView::Intersection Intersection;
    typedef Dune::MultipleCodimMultipleGeomTypeMapper<GridView,ElementLayout> ElementMapper;



public:
    IntersectionMapper(const GridView& gridview)
    : gridView_(gridview), elementMapper_(gridView_), size_(gridView_.size(1)), intersectionMapGlobal_(gridView_.size(0)), intersectionMapLocal_(gridView_.size(0))
    {
        ElementIterator eIt = gridView_.template begin<0>();

        int faceIdx = 0;

        IntersectionIterator isItEnd = gridView_.iend(*eIt);
        for (IntersectionIterator isIt = gridView_.ibegin(*eIt); isIt != isItEnd; ++isIt)
        {
            int idxInInside = isIt->indexInInside();

            standardLocalIdxMap_[idxInInside] = faceIdx;

            faceIdx++;
        }
    }

    const ElementMapper& elementMapper() const
    {
        return elementMapper_;
    }

    int map(const Element& element) const
    {
        return elementMapper_.map(element);
    }

    int map(int elemIdx, int faceIdx)
    {
        return intersectionMapGlobal_[elemIdx][faceIdx];
    }

    int map(int elemIdx, int faceIdx) const
    {
        return (intersectionMapGlobal_[elemIdx].find(faceIdx))->second;//use find() for const function!
    }

    int map(const Element& element, int faceIdx)
    {
        return intersectionMapGlobal_[map(element)][faceIdx];
    }

    int map(const Element& element, int faceIdx) const
    {
        return intersectionMapGlobal_[map(element)].find(faceIdx)->second;//use find() for const function!
    }

    int maplocal(int elemIdx, int faceIdx)
    {
        return intersectionMapLocal_[elemIdx][faceIdx];
    }

    int maplocal(int elemIdx, int faceIdx) const
    {
        return (intersectionMapLocal_[elemIdx].find(faceIdx))->second;//use find() for const function!
    }


    int maplocal(const Element& element, int faceIdx)
    {
        return intersectionMapLocal_[map(element)][faceIdx];
    }

    int maplocal(const Element& element, int faceIdx) const
    {
        return (intersectionMapLocal_[map(element)].find(faceIdx))->second;//use find() for const function!
    }

    // return number intersections
    unsigned int size () const
    {
        return size_;
    }

    unsigned int size (int elemIdx) const
    {
        return intersectionMapLocal_[elemIdx].size();
    }

    unsigned int size (const Element& element) const
    {
        return intersectionMapLocal_[map(element)].size();
    }

    void update()
    {
        elementMapper_.update();

        intersectionMapGlobal_.clear();
        intersectionMapGlobal_.resize(elementMapper_.size());
        intersectionMapLocal_.clear();
        intersectionMapLocal_.resize(elementMapper_.size());

        ElementIterator eItEnd = gridView_.template end<0>();
        for (ElementIterator eIt = gridView_.template begin<0>(); eIt != eItEnd; ++eIt)
        {
            int globalIdx = map(*eIt);

            int faceIdx = 0;
            // run through all intersections with neighbors
            IntersectionIterator isItEnd = gridView_.iend(*eIt);
            for (IntersectionIterator isIt = gridView_.ibegin(*eIt); isIt != isItEnd; ++isIt)
            {
                int indexInInside = isIt->indexInInside();
                intersectionMapLocal_[globalIdx][faceIdx] = indexInInside;

                faceIdx++;
            }
        }

        int globalIntersectionIdx = 0;
        for (ElementIterator eIt = gridView_.template begin<0>(); eIt != eItEnd; ++eIt)
        {
            int globalIdx = map(*eIt);

            int faceIdx = 0;
            // run through all intersections with neighbors
            IntersectionIterator isItEnd = gridView_.iend(*eIt);
            for (IntersectionIterator isIt = gridView_.ibegin(*eIt); isIt != isItEnd; ++isIt)
            {
                if (isIt->neighbor())
                {
                    ElementPointer neighbor = isIt->outside();
                    int globalIdxNeighbor = map(*neighbor);

                    if (eIt->level() > neighbor->level() || (eIt->level() == neighbor->level() && globalIdx < globalIdxNeighbor))
                    {

                        int faceIdxNeighbor = 0;
                        if (size(globalIdxNeighbor) == 2 * dim)
                        {
                            faceIdxNeighbor = standardLocalIdxMap_[isIt->indexInOutside()];
                        }
                        else
                        {
                            IntersectionIterator isItNEnd = gridView_.iend(*neighbor);
                            for (IntersectionIterator isItN = gridView_.ibegin(*neighbor); isItN != isItNEnd; ++isItN)
                            {
                                if (isItN->neighbor())
                                {
                                    if (isItN->outside() == eIt)
                                    {
                                        break;
                                    }
                                }
                                faceIdxNeighbor++;
                            }
                        }

                        intersectionMapGlobal_[globalIdx][faceIdx] = globalIntersectionIdx;
                        intersectionMapGlobal_[globalIdxNeighbor][faceIdxNeighbor] = globalIntersectionIdx;
                        globalIntersectionIdx ++;
                    }
                }
                else
                {
                    intersectionMapGlobal_[globalIdx][faceIdx] = globalIntersectionIdx;
                    globalIntersectionIdx ++;
                }
                faceIdx++;
            }
        }
        size_ = globalIntersectionIdx;
    }

protected:
    const GridView& gridView_;
    ElementMapper elementMapper_;
    unsigned int size_;
    std::vector<std::unordered_map<int, int> > intersectionMapGlobal_;
    std::vector<std::unordered_map<int, int> > intersectionMapLocal_;
    std::unordered_map<int, int> standardLocalIdxMap_;
};

}

#endif