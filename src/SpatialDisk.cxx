/** 
 * @file SpatialDisk.cxx
 * @brief Implementation of Function object class to represent a 2D spatial disk.
 * 
 * @author M. Wood
 *
 * $Header$
 *
 */

#include <algorithm>
#include <iostream>
#include <stdexcept>

#include "facilities/Util.h"

#include "st_stream/StreamFormatter.h"

#include "st_facilities/GaussianQuadrature.h"
#include "st_facilities/Util.h"

#include "astro/SkyDir.h"

#include "optimizers/dArg.h"
#include "optimizers/Function.h"

#include "Likelihood/Event.h"
#include "Likelihood/ExposureMap.h"
#include "Likelihood/MeanPsf.h"
#include "Likelihood/ResponseFunctions.h"
#include "Likelihood/SpatialDisk.h"
#include "Likelihood/SkyDirArg.h"

namespace {

  double disk(double x,double sigma) {
    if(x/sigma < 1.0) 
      return std::pow(sigma*sigma*M_PI,-1);
    else 
      return 0.0;
  }
}

namespace Likelihood {

double SpatialDisk::RadialIntegrand::operator()(double xp) const {

  const double s2 = std::pow(m_sigma,2);
  double dphi = 2.0*M_PI;
  if((xp+m_x)/m_sigma>1.0)
    dphi = 2.0*std::acos((std::pow(m_x,2)+std::pow(xp,2)-s2)/(2*m_x*xp));

  return xp*m_fn(m_energy,xp)*dphi/(M_PI*s2);
}

double SpatialDisk::convolve(const ResponseFunctor& fn, 
			     double energy, double x, 
			     double sigma, double err) {
  
  const double xmin = std::max(x - sigma,0.0);
  const double xmax = x + sigma;
  RadialIntegrand rIntegrand(fn,energy,x,sigma);

  int ierr;
  return st_facilities::GaussianQuadrature::dgaus8(rIntegrand, xmin,
						   xmax, err, ierr);
}


SpatialDisk::SpatialDisk() : SpatialFunction("SpatialDisk",3) {
  m_width = 1.0;
  addParam("Width", 1.0, false);
  parameter("Width").setBounds(0.0, 180.);
}

SpatialDisk::SpatialDisk(double ra, double dec, double width) 
  : SpatialFunction("SpatialDisk",3,ra,dec) {
  m_width = width;
  addParam("Width", m_width, false);
  parameter("Width").setBounds(0.0, 180.);
}

SpatialDisk::SpatialDisk(const SpatialDisk & rhs) 
  : SpatialFunction(rhs), m_width(rhs.m_width) {
}

SpatialDisk & SpatialDisk::operator=(const SpatialDisk & rhs) {
   if (this != &rhs) {
      SpatialFunction::operator=(rhs);
      m_width = rhs.m_width;
   }
   return *this;
}

SpatialDisk::~SpatialDisk() {
}

double SpatialDisk::value(const astro::SkyDir & dir) const {
   double delta = this->dir().difference(dir)*180./M_PI;
   return disk(delta,m_width)*std::pow(M_PI/180.,-2);
}

double SpatialDisk::value(double delta, double width) const {
   return disk(delta,width)*std::pow(M_PI/180.,-2);
}

double SpatialDisk::spatialResponse(const astro::SkyDir & dir, double energy, const MeanPsf& psf) const {
  double delta = dir.difference(this->dir())*180./M_PI;
  return SpatialDisk::convolve(BinnedResponseFunctor(psf),energy,delta,m_width);
}

double SpatialDisk::spatialResponse(double delta, double energy, const MeanPsf& psf) const {
  return SpatialDisk::convolve(BinnedResponseFunctor(psf),energy,delta,m_width);
}

double SpatialDisk::diffuseResponse(const ResponseFunctor& fn, double energy,
				    double separation) const {
  return convolve(fn,energy,separation,m_width);
}

double SpatialDisk::getDiffRespLimits(const astro::SkyDir & dir, 
				      double & mumin, double & mumax,
				      double & phimin, double & phimax) const {
   mumin = std::cos(dir.difference(this->dir()) + 3.*m_width*M_PI/180.);
   mumax = 1;
   phimin = 0;
   phimax = 2.*M_PI;
}

void SpatialDisk::update() {
  SpatialFunction::update();
  double width = getParam("Width").getValue();  
  m_width = width;
}

double SpatialDisk::value(const optimizers::Arg & x) const {
   const SkyDirArg & dir = dynamic_cast<const SkyDirArg &>(x);
   double offset = dir().difference(this->dir())*180./M_PI;
   return value(offset,m_width);
}

double SpatialDisk::derivByParamImp(const optimizers::Arg & x, 
                                      const std::string & parName) const {

   std::ostringstream message;
   message << "SpatialDisk: cannot take derivative wrt "
           << "parameter " << parName;
   throw std::runtime_error(message.str());
}

} // namespace Likelihood
