/** 
 * @file Source.cxx
 * @brief Source class implementation
 * @author J. Chiang
 *
 * $Header$
 */

#include "Likelihood/Source.h"

namespace Likelihood {

Source::Source() : m_name(""), m_srcType(""), m_useEdisp(false) {}

Source::Source(const Source &rhs) {
// Delegate the deep copy of m_functions to the subclasses.
   m_name = rhs.m_name;
   m_srcType = rhs.m_srcType;
   m_useEdisp = rhs.m_useEdisp;
}

} // namespace Likelihood
