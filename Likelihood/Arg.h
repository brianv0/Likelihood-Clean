/** 
 * @file Arg.h
 * @brief Declaration of Arg class
 * @author J. Chiang
 * $Header$
 */

#ifndef Likelihood_Arg_h
#define Likelihood_Arg_h

namespace Likelihood {

/** 
 * @class Arg
 *
 * @brief An abstract class that encapsulates argument type
 * information so that Function's value() and Parameter passing
 * methods can be overloaded transparently.
 *
 * @authors J. Chiang
 *    
 * $Header$
 */

class Arg {
    
public:

   Arg() {}
   
   virtual ~Arg() {}

protected:

//   Arg() {}

};

} // namespace Likelihood

#endif // Likelihood_Arg_h
