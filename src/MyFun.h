/** 
 * @file MyFun.h
 * @brief Test function declaration.
 * @author J. Chiang
 *
 * $Header$
 */

#include "Likelihood/Function.h"

namespace Likelihood {

class Arg;

/** 
 * @class MyFun
 *
 * @brief A simple test function that inherits from Function
 *
 * @author J. Chiang
 *    
 * $Header$
 */
    
class MyFun : public Function {
public:

   MyFun();
   ~MyFun(){}

   double value(Arg &) const;

   double derivByParam(Arg &x, const std::string &paramName) const
      throw(ParameterNotFound);

private:

};

} // namespace Likelihood

