/** 
 * @file Event.h
 * @brief Event class declaration
 * @author J. Chiang
 *
 * $Header$
 */


#ifndef Event_h
#define Event_h

#include <vector>
#include <string>
#include <map>
//#include <utility>

#include "astro/SkyDir.h"
#include "Likelihood/FitsImage.h"
#include "Likelihood/LikelihoodException.h"

namespace Likelihood {

class DiffuseSource;

/** 
 * @class Event
 *
 * @brief A gamma-ray event --- apparent direction, energy, arrival time, etc.
 *
 * @author J. Chiang
 *    
 * $Header$
 */

class Event {
    
public:

   Event() {}
   Event(double ra, double dec, double energy, double time, 
         double sc_ra, double sc_dec, double muZenith);
   Event(const Event &);
   virtual ~Event() {}

   astro::SkyDir getDir() const {return m_appDir;}
   astro::SkyDir getScDir() const {return m_scDir;}

   //! some useful accessor functions
   double getEnergy() const {return m_energy;}
   double getArrTime() const {return m_arrTime;}
   double getMuZenith() const {return m_muZenith;}

   //! separation in units of radians
   double getSeparation(const astro::SkyDir &dir) const 
      {return m_appDir.SkyDir::difference(dir);}

   //! return the Event specific diffuse response function 
   //! for the named diffuse component
   double diffuseResponse(double energy, 
                          const std::string &diffuseComponent) const
      throw(LikelihoodException);
    
   //! This method takes the spatial distribution of the emission for
   //! the DiffuseSource src and computes the event-specific response.
   //! See section 1 of 
   //! <a href="http://lheawww.gsfc.nasa.gov/~jchiang/SSC/like_3.ps>
   //! LikeMemo 3</a>.  The computed response is added to the
   //! m_respDiffuseSrcs map with the specified name.  sr_radius is the
   //! "source region" radius (in degrees) over which the spatial
   //! distribution of src will be integrated.
   void computeResponse(DiffuseSource &src, double sr_radius = 30.);

   //! Compute the reponse integrals for a vector of DiffuseSources
   void computeResponse(std::vector<DiffuseSource> &srcs, 
                        double sr_radius = 30.);
   
private:

   //! apparent direction, energy, arrival time, and cosine(zenith angle)
   astro::SkyDir m_appDir;
   double m_energy;
   double m_arrTime;
   double m_muZenith;
   
   //! spacecraft info at event arrival time
   astro::SkyDir m_scDir;

   //! response function data, unique to each event, and 
   //! comprising an energy redistribution function...
   typedef std::vector< std::pair<double, double> > diffuse_response;

   //! for uniform diffuse extragalactic emission,
   diffuse_response m_respEg;

   //! for the diffuse Galactic model,
   diffuse_response m_respGal;

   //! and for any number of diffuse sources,
   std::map<std::string, diffuse_response> m_respDiffuseSrcs;

   //! compute Celestial direction from (phi, mu) in Equinox-centered
   //! coordinates
   void getCelestialDir(double phi, double mu, FitsImage::EquinoxRotation
                        &eqRot, astro::SkyDir &dir);

};

} // namespace Likelihood
#endif // Event_h
