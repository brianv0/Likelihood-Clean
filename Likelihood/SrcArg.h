/** 
 * @file SrcArg.h
 * @brief Declaration of SrcArg class
 * @author J. Chiang
 *
 * $Header$
 */

#ifndef Likelihood_SrcArg_h
#define Likelihood_SrcArg_h

#include "Likelihood/Arg.h"
#include "Likelihood/Source.h"

namespace Likelihood {

/** 
 * @class SrcArg
 *
 * @brief Concrete Arg subclass for encapsulating data of type Source.
 *
 * @authors J. Chiang
 *    
 * $Header$
 */

class SrcArg : public Arg {
    
public:
   
   SrcArg(Source *src) : m_val(src) {}
   virtual ~SrcArg() {}

   Source *getValue() const {return m_val;}

private:

   Source *m_val;

};

} // namespace Likelihood

#endif // Likelihood_SrcArg_h
