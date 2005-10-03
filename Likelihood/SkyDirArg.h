/** 
 * @file SkyDirArg.h
 * @brief Declaration of SkyDirArg class
 * @author J. Chiang
 *
 * $Header$
 */

#ifndef Likelihood_SkyDirArg_h
#define Likelihood_SkyDirArg_h

#include "optimizers/Arg.h"
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

class SkyDirArg : public optimizers::Arg {
    
public:
   
   SkyDirArg(astro::SkyDir dir, double energy=100.) : 
      m_val(dir), m_energy(energy) {}

   SkyDirArg(double ra, double dec, double energy=100.) : 
      m_val(astro::SkyDir(ra, dec)), m_energy(energy) {}

   virtual ~SkyDirArg() {}

   void fetchValue(astro::SkyDir &dir) const {dir = m_val;}

   const astro::SkyDir & operator()() const {
      return m_val;
   }

   double energy() const {return m_energy;}

private:

   astro::SkyDir m_val;

   double m_energy;

};

} // namespace Likelihood

#endif // Likelihood_SkyDirArg_h
