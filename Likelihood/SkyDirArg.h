/** 
 * @file SkyDirArg.h
 * @brief Declaration of SkyDirArg class
 * @author J. Chiang
 *
 * $Header$
 */

#ifndef SkyDirArg_h
#define SkyDirArg_h

#include "Likelihood/Arg.h"
#include "astro/SkyDir.h"

namespace Likelihood {

/** 
 * @class SkyDirArg
 *
 * @brief Concrete Arg subclass for encapsulating data of type astro::SkyDir
 *
 * @authors J. Chiang
 *    
 * $Header$
 */

class SkyDirArg : public Arg {
    
public:
   
   SkyDirArg(astro::SkyDir dir) : m_val(dir) {}
   virtual ~SkyDirArg() {}

   void fetchValue(astro::SkyDir &dir) const {dir = m_val;}

private:

   astro::SkyDir m_val;

};

} // namespace Likelihood

#endif // SkyDirArg_h
