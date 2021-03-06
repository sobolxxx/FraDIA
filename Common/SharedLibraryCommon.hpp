/*!
 * \file SharedLibraryCommon.hpp
 * \brief
 *
 * \author mstefanc
 * \date 2010-05-07
 */

#ifndef SHAREDLIBRARYCOMMON_HPP_
#define SHAREDLIBRARYCOMMON_HPP_

#include <boost/preprocessor/arithmetic/inc.hpp>
#include <boost/preprocessor/if.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/repetition.hpp>
#include <boost/preprocessor/iteration/iterate.hpp>
#include <boost/concept_check.hpp>

namespace Common {

#ifndef SHARED_LIBRARY_MAX_FUNCTOR_PARAMS
#define SHARED_LIBRARY_MAX_FUNCTOR_PARAMS 10
#endif

#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32) || defined(MSC_VER)
#  define LIB_EXT ".dll"
#else
#  define LIB_EXT ".so"
#endif

typedef void* library_handle;


} //: namespace Common

#endif /* SHAREDLIBRARYCOMMON_HPP_ */
