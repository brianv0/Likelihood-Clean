/** 
 * @file ResponseFunctions.h
 * @brief A singleton class to contain the instrument response functions.
 * @author J. Chiang
 * 
 * $Header$
 */

#ifndef Likelihood_ResponseFunctions_h
#define Likelihood_ResponseFunctions_h

#include <map>

#include "latResponse/Irfs.h"

namespace Likelihood {

/** 
 * @class ResponseFunctions
 *
 * @brief This class provides global access to a map of pointers to
 * latResponse::Irfs objects.  These pointers are indexed by event
 * type, given as an integer; a map is used since the indices need not
 * be contiguous.
 *
 * @author J. Chiang
 *    
 * $Header$
 */

class ResponseFunctions {
    
public:
    
   virtual ~ResponseFunctions() {}

   static ResponseFunctions * instance();

   static void setRespPtrs(std::map<unsigned int, latResponse::Irfs *> 
                           &respPtrs) {s_respPtrs = respPtrs;}

   latResponse::Irfs * respPtr(unsigned int eventType);

   std::map<unsigned int, latResponse::Irfs *>::iterator begin()
      {return s_respPtrs.begin();}

   std::map<unsigned int, latResponse::Irfs *>::iterator end()
      {return s_respPtrs.end();}

protected:

   ResponseFunctions() {}

private:

   static ResponseFunctions * s_instance;

   static std::map<unsigned int, latResponse::Irfs *> s_respPtrs;

};

} // namespace Likelihood

#endif // Likelihood_ResponseFunctions_h
