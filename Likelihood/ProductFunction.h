/** 
 * @file ProductFunction.h
 * @brief Declaration of ProductFunction class
 * @author J. Chiang
 *
 * $Header$
 */

#ifndef Likelihood_ProductFunction_h
#define Likelihood_ProductFunction_h

#include "Likelihood/CompositeFunction.h"

namespace Likelihood {
/** 
 * @class ProductFunction
 *
 * @brief A Function that returns the product of two Functions
 *
 * @author J. Chiang
 *    
 * $Header$
 *
 */
    
class ProductFunction : public CompositeFunction {
public:

   ProductFunction(Function &a, Function &b);

   double value(Arg &x) const
      {return m_a->value(x)*m_b->value(x);}

   virtual Function* clone() const {
      return new ProductFunction(*this);
   }

protected:

   void fetchDerivs(Arg &x, std::vector<double> &derivs, bool getFree) const;

};

} // namespace Likelihood

#endif // Likelihood_ProductFunction_h
