/** 
 * @file ScData.cxx
 * @brief Implementation for the LAT spacecraft data class
 * @author J. Chiang
 * 
 * $Header$
 */

#include <cmath>

#include <algorithm>
#include <string>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "facilities/Util.h"

#include "tip/IFileSvc.h"
#include "tip/Table.h"

#include "astro/EarthCoordinate.h"

#include "Likelihood/ScData.h"

namespace Likelihood {

void ScData::readData(std::string file, bool clear) {
   facilities::Util::expandEnvVar(&file);

   m_scFile = file;

   tip::Table * scData = tip::IFileSvc::instance().editTable(file, "ext1");

   if (clear) {
      vec.clear();
   }

   double raSCX, decSCX;
   double raSCZ, decSCZ;
   tip::Table::Iterator it = scData->begin();
   tip::Table::Record & scInterval = *it;
   for ( ; it != scData->end(); ++it) {
      ScNtuple tuple;
      scInterval["start"].get(tuple.time);
      scInterval["ra_scx"].get(raSCX);
      scInterval["dec_scx"].get(decSCX);
      tuple.xAxis = astro::SkyDir(raSCX, decSCX);
      scInterval["ra_scz"].get(raSCZ);
      scInterval["dec_scz"].get(decSCZ);
      tuple.zAxis = astro::SkyDir(raSCZ, decSCZ);
// Ensure that startTimes are contiguous.
      if (vec.size() > 1 && tuple.time < vec[vec.size()-2].time) {
         std::ostringstream message;
         message << "Likelihood::ScData: "
                 << "The start times in the spacecraft data are not "
                 << "contiguous.\n"
                 << "Previous time: " << vec[vec.size()-2].time << "\n"
                 << "Current time: " << tuple.time << "\n"
                 << "Current S/C file: " << m_scFile << "\n"
                 << "Check the ordering of your S/C files." 
                 << std::endl;
         throw std::runtime_error(message.str());
      }
      tuple.inSaa = 0;
      vec.push_back(tuple);
   }
   m_tstep = vec[1].time - vec[0].time;

   delete scData;
}         

unsigned int ScData::time_index(double time) const {
   ScNtuple my_vec;
   my_vec.time = time;
   std::vector<ScNtuple>::const_iterator it
      = std::upper_bound(vec.begin(), vec.end(), my_vec, less_than_time);
   unsigned int indx = it - vec.begin() - 1;
   return indx;
}

astro::SkyDir ScData::zAxis(double time) const {
//    int indx = static_cast<int>((time - vec[0].time)/m_tstep);
//    indx = std::min(static_cast<unsigned int>(indx), vec.size()-2);
   unsigned int indx = time_index(time);
   indx = std::min(indx, vec.size()-2);
   double frac = (time - vec[indx].time)/m_tstep;
   Hep3Vector zDir = frac*(vec[indx+1].zAxis.dir() - vec[indx].zAxis.dir())
      + vec[indx].zAxis.dir();
   return astro::SkyDir(zDir.unit());
}

astro::SkyDir ScData::xAxis(double time) const {
//    int indx = static_cast<int>((time - vec[0].time)/m_tstep);
//    indx = std::min(static_cast<unsigned int>(indx), vec.size()-2);
   unsigned int indx = time_index(time);
   indx = std::min(indx, vec.size()-2);
   double frac = (time - vec[indx].time)/m_tstep;
   Hep3Vector xDir = frac*(vec[indx+1].xAxis.dir() - vec[indx].xAxis.dir())
      + vec[indx].xAxis.dir();
   return astro::SkyDir(xDir.unit());
}

std::pair<ScData::Iterator, ScData::Iterator> 
ScData::bracketInterval(double startTime, double stopTime) const {
   ScNtuple startTuple;
   startTuple.time = startTime;
   ScData::Iterator lowerBound 
      = std::lower_bound(vec.begin(), vec.end(), startTuple,
                         Likelihood::ScData::less_than_time);
   if (lowerBound == vec.end()) {
      std::ostringstream message;
      message << "Likelihood::ScData::bracketInterval:\nStart time " 
              << startTime << " is out-of-range for "
              << "existing spacecraft data time range: (" 
              << (*vec.begin()).time
              << ", " << (*(vec.end()-1)).time << ")";
      throw std::out_of_range(message.str());
   }
   if ((*lowerBound).time != startTime) --lowerBound;

   ScNtuple stopTuple;
   stopTuple.time = stopTime;
   ScData::Iterator upperBound 
      = std::upper_bound(vec.begin(), vec.end(), stopTuple,
                         Likelihood::ScData::less_than_time);
   if (upperBound == vec.end()) {
      std::ostringstream message;
      message << "Likelihood::ScData::bracketInterval:\nStop time " 
              << stopTime << " is out-of-range for "
              << "existing spacecraft data time range: (" 
              << (*vec.begin()).time
              << ", " << (*(vec.end()-1)).time << ")";
      throw std::out_of_range(message.str());
   }
   return std::make_pair(lowerBound, upperBound);
}

bool ScData::less_than_time(const ScNtuple & scDatum1, 
                            const ScNtuple & scDatum2) {
   return scDatum1.time < scDatum2.time;
}

} // namespace Likelihood
