// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*****************************************************************************
 *   See the file COPYING for full copying permissions.                      *
 *                                                                           *
 *   This program is free software: you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation, either version 3 of the License, or       *
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
 * \ingroup Properties
 * \ingroup TypeTraits
 * \author Timo Koch
 * \brief The Dumux property system, traits with inheritance
 */
#ifndef DUMUX_PROPERTY_SYSTEM_HH
#define DUMUX_PROPERTY_SYSTEM_HH

#include <tuple>
#include <type_traits>

namespace Dumux {
namespace Properties {

//! a tag to mark properties as undefined
struct UndefinedProperty {};

//! implementation details for template meta programming
namespace Detail {

//! check if a property P is defined
template<class P>
constexpr auto isDefinedProperty(int)
-> decltype(std::integral_constant<bool, !std::is_same<typename P::type, UndefinedProperty>::value>{})
{ return {}; }

//! fall back if a Property is defined
template<class P>
constexpr std::true_type isDefinedProperty(...) { return {}; }

//! check if a TypeTag inherits from other TypeTags
//! the enable_if portion of decltype is only needed for the macro hack to work, if no macros are in use anymore it can be removed,
//! i.e. then trailing return type is then -> decltype(std::declval<typename T::InheritsFrom>(), std::true_type{})
template<class T>
constexpr auto hasParentTypeTag(int)
-> decltype(std::declval<typename T::InheritsFrom>(), std::enable_if_t<!std::is_same<typename T::InheritsFrom, void>::value, int>{}, std::true_type{})
{ return {}; }

//! fall back if a TypeTag doesn't inherit
template<class T>
constexpr std::false_type hasParentTypeTag(...) { return {}; }

//! helper alias to concatenate multiple tuples
template<class ...Tuples>
using ConCatTuples = decltype(std::tuple_cat(std::declval<Tuples>()...));

//! helper struct to get the first property that is defined in the TypeTag hierarchy
template<class TypeTag, template<class,class> class Property, class TTagList>
struct GetDefined;

//! helper struct to iteratre over the TypeTag hierarchy
template<class TypeTag, template<class,class> class Property, class TTagList, class Enable>
struct GetNextTypeTag;

template<class TypeTag, template<class,class> class Property, class LastTypeTag>
struct GetNextTypeTag<TypeTag, Property, std::tuple<LastTypeTag>, std::enable_if_t<hasParentTypeTag<LastTypeTag>(int{}), void>>
{ using type = typename GetDefined<TypeTag, Property, typename LastTypeTag::InheritsFrom>::type; };

template<class TypeTag, template<class,class> class Property, class LastTypeTag>
struct GetNextTypeTag<TypeTag, Property, std::tuple<LastTypeTag>, std::enable_if_t<!hasParentTypeTag<LastTypeTag>(int{}), void>>
{ using type = UndefinedProperty; };

template<class TypeTag, template<class,class> class Property, class FirstTypeTag, class ...Args>
struct GetNextTypeTag<TypeTag, Property, std::tuple<FirstTypeTag, Args...>, std::enable_if_t<hasParentTypeTag<FirstTypeTag>(int{}), void>>
{ using type = typename GetDefined<TypeTag, Property, ConCatTuples<typename FirstTypeTag::InheritsFrom, std::tuple<Args...>>>::type; };

template<class TypeTag, template<class,class> class Property, class FirstTypeTag, class ...Args>
struct GetNextTypeTag<TypeTag, Property, std::tuple<FirstTypeTag, Args...>, std::enable_if_t<!hasParentTypeTag<FirstTypeTag>(int{}), void>>
{ using type = typename GetDefined<TypeTag, Property, std::tuple<Args...>>::type; };

template<class TypeTag, template<class,class> class Property, class LastTypeTag>
struct GetDefined<TypeTag, Property, std::tuple<LastTypeTag>>
{
     using LastType = Property<TypeTag, LastTypeTag>;
     using type = std::conditional_t<isDefinedProperty<LastType>(int{}), LastType,
                                     typename GetNextTypeTag<TypeTag, Property, std::tuple<LastTypeTag>, void>::type>;
};

template<class TypeTag, template<class,class> class Property, class FirstTypeTag, class ...Args>
struct GetDefined<TypeTag, Property, std::tuple<FirstTypeTag, Args...>>
{
     using FirstType = Property<TypeTag, FirstTypeTag>;
     using type = std::conditional_t<isDefinedProperty<FirstType>(int{}), FirstType,
                                     typename GetNextTypeTag<TypeTag, Property, std::tuple<FirstTypeTag, Args...>, void>::type>;
};

//! helper struct to extract get the Property specilization given a TypeTag, asserts that the property is defined
template<class TypeTag, template<class,class> class Property>
struct GetPropImpl
{
    using type = typename Detail::GetDefined<TypeTag, Property, std::tuple<TypeTag>>::type;
    static_assert(!std::is_same<type, UndefinedProperty>::value, "Property is undefined!");
};

} // end namespace Detail
} // end namespace Property

//! get the type of a property (equivalent to old macro GET_PROP(...))
template<class TypeTag, template<class,class> class Property>
using GetProp = typename Properties::Detail::GetPropImpl<TypeTag, Property>::type;

//! get the type alias defined in the property (equivalent to old macro GET_PROP_TYPE(...))
template<class TypeTag, template<class,class> class Property>
using GetPropType = typename Properties::Detail::GetPropImpl<TypeTag, Property>::type::type;

//! get the value data member of a property
template<class TypeTag, template<class,class> class Property>
constexpr auto getPropValue() { return Properties::Detail::GetPropImpl<TypeTag, Property>::type::value; }

} // end namespace Dumux

#endif
