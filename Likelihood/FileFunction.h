/** 
 * @file FileFunction.h
 * @brief Declaration for the FileFunction class
 * @author J. Chiang
 *
 * $Header$
 */

#ifndef Likelihood_FileFunction_h
#define Likelihood_FileFunction_h

#include <string>
#include <vector>

#include "optimizers/Arg.h"
#include "optimizers/Function.h"

namespace Likelihood {

/** 
 * @class FileFunction
 *
 * @brief This reads in a tabulated function from an ascii file containing
 * two columns of data.
 *
 */
    
class FileFunction : public optimizers::Function {

public:

   FileFunction(double Normalization=1);
   
   virtual Function * clone() const {
      return new FileFunction(*this);
   }

   void readFunction(const std::string & filename);

   const std::string & filename() const {
      return m_filename;
   }

   void setSpectrum(const std::vector<double> & energy,
                    const std::vector<double> & dnde);

   const std::vector<double> & log_energy() const;
   const std::vector<double> & log_dnde() const;

protected:

   double value(const optimizers::Arg & x) const;

   double derivByParamImp(const optimizers::Arg & x, 
                          const std::string & paramName) const;

private:

   double interpolateFlux(double logEnergy) const;

   std::vector<double> m_x;
   std::vector<double> m_y;

   std::string m_filename;

};

} // namespace Likelihood

#endif // Likelihood_FileFunction_h
