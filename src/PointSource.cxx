/** 
 * @file PointSource.cxx
 * @brief PointSource class implementation
 *
 * $Header$
 */

#include <vector>
#include <string>
#include <cmath>

#include "facilities/Util.h"

#include "astro/SkyDir.h"

#include "optimizers/dArg.h"

#include "latResponse/Irfs.h"
#include "latResponse/../src/Glast25.h"

#include "map_tools/Exposure.h"

#include "Likelihood/ResponseFunctions.h"
#include "Likelihood/PointSource.h"
#include "Likelihood/ScData.h"
#include "Likelihood/RoiCuts.h"
#include "Likelihood/TrapQuad.h"

namespace {
}

namespace Likelihood {

map_tools::Exposure * PointSource::s_exposure = 0;

bool PointSource::s_haveStaticMembers = false;
std::vector<double> PointSource::s_energies;

PointSource::PointSource(const PointSource &rhs) : Source(rhs) {
// make a deep copy
   m_dir = rhs.m_dir;
   m_functions["Position"] = &m_dir;

   m_spectrum = rhs.m_spectrum->clone();
   m_functions["Spectrum"] = m_spectrum;

   m_exposure = rhs.m_exposure;
   m_srcType = rhs.m_srcType;
}

void PointSource::readExposureCube(std::string expCubeFile) {
   facilities::Util::expandEnvVar(&expCubeFile);
   s_exposure = new map_tools::Exposure(expCubeFile);
}

double PointSource::fluxDensity(double energy, double time,
                                const astro::SkyDir &dir,
                                int eventType) const {

// Scale the energy spectrum by the psf value and the effective area
// and convolve with the energy dispersion (now a delta-function in
// energy), all of which are functions of time and spacecraft attitude
// and orbital position.

   optimizers::dArg energy_arg(energy);
   double spectrum = (*m_spectrum)(energy_arg);

   return ResponseFunctions::totalResponse(time, energy, energy, 
                                           m_dir.getDir(), dir, eventType)
      *spectrum;
                                           
}

double PointSource::fluxDensityDeriv(double energy, double time,
                                     const astro::SkyDir &dir,
                                     int eventType,
                                     const std::string &paramName) const {
// For now, just implement for spectral Parameters and neglect
// the spatial ones, "longitude" and "latitude"

   if (paramName == "Prefactor") {
      return fluxDensity(energy, time, dir, eventType)
         /m_spectrum->getParamValue("Prefactor");
   } else {
      optimizers::dArg energy_arg(energy);
      return ResponseFunctions::totalResponse(time, energy, energy,
                                              m_dir.getDir(), dir, eventType)
         *m_spectrum->derivByParam(energy_arg, paramName);
   }
}

double PointSource::Npred() {
   optimizers::Function *specFunc = m_functions["Spectrum"];

// Evaluate the Npred integrand at the abscissa points contained in
// s_energies
   
   std::vector<double> NpredIntegrand(s_energies.size());
   for (unsigned int k = 0; k < s_energies.size(); k++) {
      optimizers::dArg eArg(s_energies[k]);
      NpredIntegrand[k] = (*specFunc)(eArg)*m_exposure[k];
   }
   TrapQuad trapQuad(s_energies, NpredIntegrand);
   return trapQuad.integral();
}

double PointSource::NpredDeriv(const std::string &paramName) {
   optimizers::Function *specFunc = m_functions["Spectrum"];

   if (paramName == std::string("Prefactor")) {
      return Npred()/specFunc->getParamValue("Prefactor");
   } else {  // loop over energies and fill integrand vector
      std::vector<double> myIntegrand(s_energies.size());
      for (unsigned int k = 0; k < s_energies.size(); k++) {
         optimizers::dArg eArg(s_energies[k]);
         myIntegrand[k] = specFunc->derivByParam(eArg, paramName)
            *m_exposure[k];
      }
      TrapQuad trapQuad(s_energies, myIntegrand);
      return trapQuad.integral();
   }
}

void PointSource::computeExposure(bool verbose) {
   if (s_exposure == 0) {
// Exposure time hypercube is not available, so perform sums using
// ScData.
      computeExposure(s_energies, m_exposure, verbose);
   } else {
      computeExposureWithHyperCube(s_energies, m_exposure, verbose);
   }
   if (verbose) {
      for (unsigned int i = 0; i < s_energies.size(); i++) {
         std::cout << s_energies[i] << "  " << m_exposure[i] << std::endl;
      }
   }
}

void PointSource::computeExposureWithHyperCube(std::vector<double> &energies, 
                                               std::vector<double> &exposure, 
                                               bool verbose) {
   if (!s_haveStaticMembers) {
      makeEnergyVector();
      s_haveStaticMembers = true;
   }
   exposure.clear();

   astro::SkyDir srcDir = getDir();
   if (verbose) {
      std::cerr << "Computing exposure at (" 
                << srcDir.ra() << ", " 
                << srcDir.dec() << ")";
   }
   for (std::vector<double>::const_iterator it = energies.begin();
        it != energies.end(); it++) {
      if (verbose) std::cerr << ".";
      PointSource::Aeff aeff(*it, srcDir);
      double exposure_value = (*s_exposure)(srcDir, aeff);
      exposure.push_back(exposure_value);
   }
   if (verbose) std::cerr << "!" << std::endl;
}

void PointSource::computeExposure(std::vector<double> &energies,
                                  std::vector<double> &exposure,
                                  bool verbose) {
   ScData *scData = ScData::instance();
   RoiCuts *roiCuts = RoiCuts::instance();

// Don't compute anything if there is no ScData.
   if (scData->vec.size() == 0) return;

   if (!s_haveStaticMembers) {
      makeEnergyVector();
      s_haveStaticMembers = true;
   }

// Initialize the exposure vector with zeros
   exposure = std::vector<double>(energies.size(), 0);

   if (verbose) {
      std::cerr << "Computing exposure at (" 
                << getDir().ra() << ", " 
                << getDir().dec() << ")";
   }
   unsigned int npts = scData->vec.size()-1;
   for (unsigned int it = 0; it < npts; it++) {
      if (npts/20 > 0 && ((it % (npts/20)) == 0) && verbose) std::cerr << ".";
      bool includeInterval = true;

// Check if this interval passes the time cuts
      std::vector< std::pair<double, double> > timeCuts;
      roiCuts->getTimeCuts(timeCuts);
      for (unsigned int itcut = 0; itcut < timeCuts.size(); itcut++) {
         if (scData->vec[it].time < timeCuts[itcut].first ||
             scData->vec[it].time > timeCuts[itcut].second) {
            includeInterval = false;
            break;
         }
      }

// Check for SAA passage
      if (scData->vec[it].inSaa) includeInterval = false;

// Compute the inclination and check if it's within response matrix
// cut-off angle
      double inc = getSeparation(scData->vec[it].zAxis)*180/M_PI;
//       if (inc > latResponse::Glast25::incMax()) includeInterval = false;
      if (inc > 90.) includeInterval = false;

// Having checked for relevant constraints, add the exposure
// contribution for each energy
      if (includeInterval) {
         for (unsigned int k = 0; k < energies.size(); k++) {
            double time = (scData->vec[it+1].time + scData->vec[it].time)/2.;
            exposure[k] += sourceEffArea(energies[k], time)
               *(scData->vec[it+1].time - scData->vec[it].time);
         }
      }
   }
   if (verbose) std::cerr << "!" << std::endl;
}

void PointSource::makeEnergyVector(int nee) {
   RoiCuts *roiCuts = RoiCuts::instance();
   
// set up a logrithmic grid of energies for doing the integral over 
// the spectrum
   double emin = (roiCuts->getEnergyCuts()).first;
   double emax = (roiCuts->getEnergyCuts()).second;
   double estep = log(emax/emin)/(nee-1);
   
   s_energies.reserve(nee);
   for (int i = 0; i < nee; i++)
      s_energies.push_back(emin*exp(i*estep));
}

double PointSource::sourceEffArea(double energy, double time) const {
   Likelihood::ScData * scData = Likelihood::ScData::instance();

   astro::SkyDir zAxis = scData->zAxis(time);
   astro::SkyDir xAxis = scData->xAxis(time);

   const astro::SkyDir & srcDir = m_dir.getDir();

   Likelihood::PointSource::Aeff aeff(energy, srcDir);

   double cos_theta = zAxis()*const_cast<astro::SkyDir&>(srcDir)();

   return aeff(cos_theta);
}

double PointSource::Aeff::operator()(double cos_theta) const {
   double theta = acos(cos_theta)*180./M_PI;
   static double phi;

   Likelihood::RoiCuts *roiCuts = Likelihood::RoiCuts::instance();
   std::vector<latResponse::AcceptanceCone *> cones;
   cones.push_back(const_cast<latResponse::AcceptanceCone *>
                   (&(roiCuts->extractionRegion())));

   Likelihood::ResponseFunctions * respFuncs 
      = Likelihood::ResponseFunctions::instance();

   double myEffArea = 0;
   std::map<unsigned int, latResponse::Irfs *>::iterator respIt
      = respFuncs->begin();

   for ( ; respIt != respFuncs->end(); respIt++) {
      latResponse::IPsf *psf = respIt->second->psf();
      latResponse::IAeff *aeff = respIt->second->aeff();

      double psf_val = psf->angularIntegral(m_energy, m_srcDir,
                                            theta, phi, cones);
      double aeff_val = aeff->value(m_energy, theta, phi);

      myEffArea += psf_val*aeff_val;
   }
   return myEffArea;
}

} // namespace Likelihood
