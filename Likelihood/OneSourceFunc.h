/**
 * @file OneSourceFunc.h
 * @brief Extended likelihood function for one source
 *
 * @author P. Nolan
 *
 * $Header$
 */

#ifndef Likelihood_OneSourceFunc_h
#define Likelihood_OneSourceFunc_h

#include "Likelihood/Event.h"
#include "Likelihood/Source.h"
#include <vector>
#include "optimizers/Function.h"
#include "optimizers/Arg.h"
#include "optimizers/Exception.h"
#include "optimizers/ParameterNotFound.h"

#include "optimizers/Statistic.h"

namespace Likelihood {

   /**
    * @class OneSourceFunc
    * @brief Extended likelihood function for one source.
    * @author P. Nolan
    *
    * $Header$
    */
  
//   class OneSourceFunc: public optimizers::Function {
  class OneSourceFunc: public optimizers::Statistic {
    
  public:

    OneSourceFunc(Source * src,  // The Source of interest
		  const std::vector<Event>& events, 
		  std::vector<double> * weights = NULL);

    virtual double value(optimizers::Arg& arg) const;
    virtual double derivByParam(optimizers::Arg& x, 
				const std::string& paramName) const;
    virtual std::vector<double>::const_iterator 
      setFreeParamValues_(std::vector<double>::const_iterator it);
    virtual std::vector<double>::const_iterator
      setParamValues_(std::vector<double>::const_iterator);
//    virtual Function *clone() const {return new OneSourceFunc(*this);}
    virtual optimizers::Statistic *clone() const 
        {return new OneSourceFunc(*this);}
    virtual void setParams(std::vector<optimizers::Parameter>&)
      throw(optimizers::Exception, optimizers::ParameterNotFound);
    void setEpsW(double);
    void setEpsF(double);

     virtual double value() const {
        optimizers::Arg dummy;
        return value(dummy);
     }

     virtual void getFreeDerivs(optimizers::Arg &x, 
                                std::vector<double> &derivs) const {
        Function::getFreeDerivs(x, derivs);
     }

     virtual void getFreeDerivs(std::vector<double> &derivs) const {
        optimizers::Arg dummy;
        getFreeDerivs(dummy, derivs);
     }

  protected:

    void syncParams(void);
    
  private:
    
    Source * m_src;
     const std::vector<Event>& m_events;
    std::vector<double> * m_weights;
    double m_epsw;
    double m_epsf;
    
  };  // class OneSourceFunc
} // namespace Likelihood

#endif // Likelihood_OneSourceFunc_h
