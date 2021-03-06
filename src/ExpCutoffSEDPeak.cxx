/**
 * @file ExpCutoffSEDPeak.cxx
 * @brief Implementation for the ExpCutoff with SED peak energy and 
 * flux as variables
 * @author Rolf Buehler
 *
 * $Header$
 */

#include <cmath>

#include <iostream>
#include <string>
#include <vector>

#include "optimizers/dArg.h"
#include "optimizers/ParameterNotFound.h"

#include "Likelihood/ExpCutoffSEDPeak.h"

namespace Likelihood {

ExpCutoffSEDPeak::
ExpCutoffSEDPeak(double Fpeak, double Index, double Epeak) 
   : optimizers::Function("ExpCutoffSEDPeak", 3, "Fpeak") {
   addParam("Fpeak", Fpeak, true);
   addParam("Index", Index, true);
   addParam("Epeak", Epeak, true);
}

double ExpCutoffSEDPeak::value(const optimizers::Arg &xarg) const {
   double x = dynamic_cast<const optimizers::dArg &>(xarg).getValue();
   //std::cout<<"In ::value"<<std::endl;

// assume a standard ordering for the parameters
   enum ParamTypes {Fpeak, Index, Epeak};

   std::vector<optimizers::Parameter> my_params;
   getParams(my_params);

   double fpeak = my_params[Fpeak].getTrueValue();
   double index = my_params[Index].getTrueValue();
   double epeak = my_params[Epeak].getTrueValue();
   
   double value = fpeak/pow(epeak,2) * pow(x/epeak,index) * pow(exp(1-x/epeak),index+2);
   
   //std::cout<< "For values: "<<x<<" "<<fpeak<<" "<<index<<" "<<epeak<<" returning:";
   //std::cout<< value<<std::endl;
   return value;
}

double ExpCutoffSEDPeak::derivByParamImp(const optimizers::Arg & xarg,
                                         const std::string & paramName) const {
   double x = dynamic_cast<const optimizers::dArg &>(xarg).getValue();

   enum ParamTypes {Fpeak, Index, Epeak};

   std::vector<optimizers::Parameter> my_params;
   getParams(my_params);

   double fpeak = my_params[Fpeak].getTrueValue();
   double index = my_params[Index].getTrueValue();
   double epeak = my_params[Epeak].getTrueValue();

   int iparam = -1;
   for (unsigned int i = 0; i < my_params.size(); i++) {
      if (paramName == my_params[i].getName()) iparam = i;
   }

   if (iparam == -1) {
      throw optimizers::ParameterNotFound(paramName, getName(),
                                          "ExpCutoffSEDPeak::derivByParam");
   }
	//std::cout<< "Dev by "<<iparam<<",for values: "<<x<<" "<<fpeak<<" "<<index<<" "<<epeak<<" returning:";
   switch (iparam){
   case Fpeak:
      //std::cout<<value(xarg) / fpeak*my_params[Fpeak].getScale()<<std::endl;
      return value(xarg) / fpeak *my_params[Fpeak].getScale();
      break;
   case Index:
      //std::cout<< value(xarg) * (log(x/epeak)+(1-x/epeak))*my_params[Index].getScale()<<std::endl;
      return value(xarg) * (log(x/epeak)+(1-x/epeak))*my_params[Index].getScale();
      break;
   case Epeak:
      //std::cout<< value(xarg) *(pow(1-x/epeak,index+1)*(index+2)*x/pow(epeak,2) - index/epeak )*my_params[Epeak].getScale()<<std::endl;
      //return value(xarg) *(pow(1-x/epeak,index+1)*(index+2)*x/pow(epeak,2) - index/epeak )*my_params[Epeak].getScale();
      //std::cout<<value(xarg) * ((index+2)*x/pow(epeak,2)-(index+2))*my_params[Epeak].getScale()<<std::endl;
//      return value(xarg) * ((index+2)*x/pow(epeak,2)-(index+2))*my_params[Epeak].getScale();
      return (-value(xarg)*(index+2.)*(1 - x/epeak)/epeak
              *my_params[Epeak].getScale());
      break;
   default:
      break;
   }
   
   return 0;
}

}
