/*
 * @file PowerLawSuperExpCutoff.h
 * @brief Declaration for the PowerLawSuperExpCutoff Function class
 * @author Damien Parent
 *
 * $Header$
 */

#ifndef Likelihood_PowerLawSuperExpCutoff_h
#define Likelihood_PowerLawSuperExpCutoff_h

#include "optimizers/Function.h"
#include "optimizers/Arg.h"

namespace Likelihood {

  /*
   * @class PowerLawSuperExpCutoff
   *
   * @brief Power Law with Super Exponential Cutoff function
   *
   * @author Damien Parent
   *
   */
  
  class PowerLawSuperExpCutoff : public optimizers::Function {
    
  public:
    
    PowerLawSuperExpCutoff(double Prefactor=10.,
			   double Index1=-2.1, 
			   double Scale=1000.,
			   double Cutoff=10000.,
			   double Index2=2.);
    
    virtual optimizers::Function * clone() const {
      return new PowerLawSuperExpCutoff(*this);
    }
    
  protected:

    double value(const optimizers::Arg &) const;
    
    double derivByParamImp(const optimizers::Arg & x,
                           const std::string &paramName) const;
    
  };
  
} // namespace Likelihood

#endif // Likelihood_PowerLawSuperExpCutoff_h
