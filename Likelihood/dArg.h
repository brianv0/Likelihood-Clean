/** 
 * @file dArg.h
 * @brief Declaration of dArg class
 * @author J. Chiang
 *
 * $Header$
 */

#ifndef Likelihood_dArg_h
#define Likelihood_dArg_h

#include "Likelihood/Arg.h"

namespace Likelihood {

/** 
 * @class dArg
 *
 * @brief Concrete Arg subclass for encapsulating data of type double.
 *
 * @authors J. Chiang
 *    
 * $Header$
 */

class dArg : public Arg{
    
public:
   
   dArg(double x) : m_val(x) {}
   virtual ~dArg() {}

   double getValue() {return m_val;}

private:

   double m_val;

};

} // namespace Likelihood

#endif // Likelihood_dArg_h
