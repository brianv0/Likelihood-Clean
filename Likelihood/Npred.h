/** 
 * @file Npred.h
 * @brief Declaration of Npred class
 * @author J. Chiang
 *
 * $Header$
 */

#ifndef Likelihood_Npred_h
#define Likelihood_Npred_h

#include "optimizers/Function.h"
#include "Likelihood/Source.h"
#include "Likelihood/SrcArg.h"

namespace Likelihood {

/** 
 * @class Npred
 *
 * @brief This class encapsulates the Npred methods of Sources in a
 * Function context.
 *  
 * @author J. Chiang
 *    
 * $Header$
 */

class Npred : public optimizers::Function {
    
public:

   Npred() {m_genericName = "Npred";}
   virtual ~Npred() {}

   double value(optimizers::Arg &) const;
   double derivByParam(optimizers::Arg &, const std::string &) const;

protected:

   Npred * clone() const {
      return new Npred(*this);
   }

private:

   void fetchDerivs(optimizers::Arg &, std::vector<double> &derivs, 
                    bool getFree) const;
   void buildParameterVector(optimizers::Arg &);

};

} // namespace Likelihood

#endif // Likelihood_Npred_h
