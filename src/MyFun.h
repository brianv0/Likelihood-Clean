#include "../Likelihood/Function.h"

namespace Likelihood {

/** 
 * @class MyFun
 *
 * @brief a simple test function that inherits from Function
 *
 * @author J. Chiang
 *    
 * $Header$
 */
    
class MyFun : public Function {
public:

   MyFun();
   virtual double value(double) const;
   virtual double operator()(double x) const {return value(x);};
   virtual double derivByParam(double, const std::string &paramName) const;

private:
};

} // namespace Likelihood

