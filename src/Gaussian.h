/** @file Gaussian.h
 * @brief Gaussian class declaration
 * @author J. Chiang
 *
 * $Header$
 */

#include "Likelihood/Function.h"
#include "Likelihood/Arg.h"

namespace Likelihood {
/** 
 * @class Gaussian
 *
 * @brief A 1D Gaussian function
 *
 * @author J. Chiang
 *    
 * $Header$
 */
    
class Gaussian : public Function {
public:

   Gaussian(){init(0, -2, 1);}
   Gaussian(double Prefactor, double Mean, double Sigma)
      {init(Prefactor, Mean, Sigma);}

   double value(Arg &) const;

   double derivByParam(Arg &, const std::string &paramName) const;

   double integral(Arg &xmin, Arg &xmax) const;

private:

   void init(double Prefactor, double Mean, double Sigma);
   double erfcc(double x) const;

};

} // namespace Likelihood

