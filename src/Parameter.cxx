/** @file Parameter.cxx
 * @brief Parameter class implementation
 * @author J. Chiang
 *
 * $Header$
 */

#include <vector>
#include <string>

#include "Likelihood/Parameter.h"

namespace Likelihood {

//! return bounds as a pair
std::pair<double, double> Parameter::getBounds() {
   std::pair<double, double> my_Bounds(m_minValue, m_maxValue);
   return my_Bounds;
}

} // namespace Likelihood
