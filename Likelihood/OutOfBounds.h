/**
 * @file OutOfBounds.h
 * @brief Declaration/definition of OutOfBounds exception class
 * @author J. Chiang
 * $Header$
 */

#ifndef Likelihood_OutOfBounds_h
#define Likelihood_OutOfBounds_h

#include "Likelihood/LikelihoodException.h"

namespace Likelihood {

/**
 * @class OutOfBounds
 *
 * @brief Exception class used by Parameter objects to help ensure
 * set[True]Value and setBounds methods behave consistently with
 * regard to existing values.
 *
 * @author J. Chiang
 *
 * $Header$
 */

class OutOfBounds : public LikelihoodException {

public:
   OutOfBounds(const std::string &errorString, double value, 
               double minValue, double maxValue, int code) : 
      LikelihoodException(errorString, code), m_value(value), 
      m_minValue(minValue), m_maxValue(maxValue) {}

   ~OutOfBounds() {}
   
   double value() {return m_value;}
   double minValue() {return m_minValue;}
   double maxValue() {return m_maxValue;}
   
   enum ERROR_CODES {VALUE_ERROR, BOUNDS_ERROR};

private:

   double m_value;
   double m_minValue;
   double m_maxValue;

};

} // namespace Likelihood

#endif // Likelihood_OutOfBounds_h
