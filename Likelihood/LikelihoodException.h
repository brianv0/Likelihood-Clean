/**
 * @file LikelihoodException.h
 * @brief Exception class for Likelihood
 * @author P. Nolan
 *
 * $Header$
 */

#ifndef LIKELIHOOD_EXCEPTION_H
#define LIKELIHOOD_EXCEPTION_H
#include <exception>
#include <string>

namespace Likelihood {
/**
 * @class LikelihoodException
 *
 * @brief Exception class for Likelihood
 *
 * @author P. Nolan
 *
 * $Header$
 */

  class LikelihoodException: public std::exception {
  public:
    LikelihoodException() {}
    LikelihoodException(std::string errorString, int code=0) : 
      m_what(errorString), m_code(code) 
      {}
    virtual ~LikelihoodException() throw() {}
    virtual const char *what() const throw() {return m_what.c_str();}
    virtual const int code() const {return m_code;}
  protected:
    std::string m_what;
    int m_code;
  };
}
#endif //LIKELIHOOD_EXCEPTION_H
