/** 
 * @file Source.cxx
 * @brief Source class implementation
 * @author J. Chiang
 *
 * $Header$
 */

#include "Likelihood/Source.h"

namespace Likelihood {

Source::Source() {}

Source::Source(const Source &rhs) {
// Delegate deep copy of m_functions to the subclasses.
   m_name = rhs.m_name;
   m_useEdisp = rhs.m_useEdisp;
}

} // namespace Likelihood
