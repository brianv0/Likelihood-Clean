/**
 * @file AppBase.h
 * @brief Base class for Likelihood applications providing common
 * functionality.
 * @author J. Chiang
 *
 * $Header$
 */

#ifndef Likelihood_AppBase
#define Likelihood_AppBase

#include <string>

#include "optimizers/FunctionFactory.h"

namespace hoops {
   class IParGroup;
}

//#include "hoops/hoops_par_group.h"

namespace Likelihood {

/**
 * @class AppBase
 * @brief A base class for Likelihood applications.
 *
 * @author J. Chiang
 *
 * $Header$
 */

class AppBase {

public:

   virtual ~AppBase() {}

   virtual void run() = 0;

protected:

   AppBase(hoops::IParGroup & pars) : m_pars(pars) {
      prepareFunctionFactory();
      setRoi();
      readScData();
      createResponseFuncs();
   }

   hoops::IParGroup & m_pars;
   optimizers::FunctionFactory m_funcFactory;

   virtual void prepareFunctionFactory();
   virtual void setRoi();
   virtual void readScData();
   virtual void createResponseFuncs();
   virtual void readExposureMap();

};

} // namespace Likelihood

#endif // Likelihood_AppBase
