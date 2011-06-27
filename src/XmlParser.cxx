/**
 * @file XmlParser.cxx
 * @brief Declaration of XmlParser::s_instance
 * @author J. Chiang
 *
 * $Header$
 */

#define ST_DLL_EXPORTS
#include "Likelihood/XmlParser.h"
#undef ST_DLL_EXPORTS

namespace Likelihood {

xmlBase::XmlParser * XmlParser::s_instance(0);

xmlBase::XmlParser * XmlParser_instance() {
   return XmlParser::instance();
}

} // namespace Likelihood
