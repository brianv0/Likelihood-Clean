/** 
 * @file LogLike.cxx
 * @brief LogLike class implementation
 * @author J. Chiang
 *
 * $Header$
 */

#include <cmath>
#include <ctime>

#include <algorithm>
#include <fstream>
#include <string>
#include <vector>

#include "st_stream/StreamFormatter.h"

#include "Likelihood/DiffuseSource.h"
#include "Likelihood/LogLike.h"
#include "Likelihood/Npred.h"
#include "Likelihood/SrcArg.h"

namespace Likelihood {

LogLike::LogLike(const Observation & observation) 
   : SourceModel(observation), m_nevals(0) {
   deleteAllSources();
}

double LogLike::value(optimizers::Arg&) const {
   std::clock_t start = std::clock();
   const std::vector<Event> & events = m_observation.eventCont().events();
   double my_value(0);

   findFreeSrcs();
   
// The "data sum"
   for (unsigned int j = 0; j < events.size(); j++) {
      my_value += logSourceModel(events.at(j));
   }

// The "model integral", a sum over Npred for each source
   std::map<std::string, Source *>::const_iterator srcIt = m_sources.begin();
   for ( ; srcIt != m_sources.end(); ++srcIt) {
      SrcArg sArg(srcIt->second);
      my_value -= m_Npred(sArg);
   }
   st_stream::StreamFormatter formatter("LogLike", "value", 4);
   formatter.info() << m_nevals << "  "
                    << my_value << "  "
                    << std::clock() - start << std::endl;
   m_nevals++;
   return my_value;
}

double LogLike::logSourceModel(const Event & event) const {
//    double my_value(0);
//    std::map<std::string, Source *>::const_iterator 
//       source(m_sources.begin());
//    for ( ; source != m_sources.end(); ++source) {
//       double fluxDens(source->second->fluxDensity(event));
//       my_value += fluxDens;
//    }
   for (size_t i = 0; i < m_freeSrcs.size(); i++) {
      const_cast<Event &>(event).updateModelSum(*m_freeSrcs.at(i));
   }
   double my_value(event.modelSum());
   if (my_value > 0) {
      return std::log(my_value);
   }
   return 0;
}

void LogLike::getLogSourceModelDerivs(const Event & event,
                                      std::vector<double> & derivs) const {
   derivs.clear();
   derivs.reserve(getNumFreeParams());
   double srcSum = std::exp(logSourceModel(event));

   std::map<std::string, Source *>::const_iterator source = m_sources.begin();
   for ( ; source != m_sources.end(); ++source) {
      Source::FuncMap srcFuncs = source->second->getSrcFuncs();
      Source::FuncMap::const_iterator func_it = srcFuncs.begin();
      for (; func_it != srcFuncs.end(); func_it++) {
         std::vector<std::string> paramNames;
         (*func_it).second->getFreeParamNames(paramNames);
         for (unsigned int j = 0; j < paramNames.size(); j++) {
            derivs.push_back(
               source->second->fluxDensityDeriv(event, paramNames[j])/srcSum
               );
         }
      }
   }
}

void LogLike::getFreeDerivs(optimizers::Arg&,
                            std::vector<double> &freeDerivs) const {
// Retrieve the free derivatives for the log(SourceModel) part
   const std::vector<Event> & events = m_observation.eventCont().events();

   std::vector<double> logSrcModelDerivs(getNumFreeParams(), 0);
   for (unsigned int j = 0; j < events.size(); j++) {
      std::vector<double> derivs;
      getLogSourceModelDerivs(events[j], derivs);
      for (unsigned int i = 0; i < derivs.size(); i++) {
         logSrcModelDerivs[i] += derivs[i];
      }
   }

// The free derivatives for the Npred part must be appended 
// for each Source in m_sources.
   std::vector<double> NpredDerivs;
   NpredDerivs.reserve(getNumFreeParams());

   std::map<std::string, Source *>::const_iterator srcIt = m_sources.begin();
   for ( ; srcIt != m_sources.end(); ++srcIt) {
      SrcArg sArg(srcIt->second);
      std::vector<double> derivs;
      m_Npred.getFreeDerivs(sArg, derivs);
      for (unsigned int i = 0; i < derivs.size(); i++) {
         NpredDerivs.push_back(derivs[i]);
      }
   }

   freeDerivs.reserve(NpredDerivs.size());
   freeDerivs.clear();
   for (unsigned int i = 0; i < NpredDerivs.size(); i++) {
      freeDerivs.push_back(logSrcModelDerivs[i] - NpredDerivs[i]);
   }
}

void LogLike::addSource(Source * src) {
   SourceModel::addSource(src);
   const std::vector<Event> & events = m_observation.eventCont().events();
   for (size_t j = 0; j < events.size(); j++) {
      const_cast<std::vector<Event> &>(events).at(j).updateModelSum(*src);
   }
}

Source * LogLike::deleteSource(const std::string & srcName) {
   const std::vector<Event> & events = m_observation.eventCont().events();
   for (size_t j = 0; j < events.size(); j++) {
      const_cast<std::vector<Event> &>(events).at(j).deleteSource(srcName);
   }
   return SourceModel::deleteSource(srcName);
}

void LogLike::getEvents(std::string event_file) {
   EventContainer & eventCont =
      const_cast<EventContainer &>(m_observation.eventCont());
   eventCont.getEvents(event_file);
}

void LogLike::computeEventResponses(double sr_radius) {
   std::vector<DiffuseSource *> diffuse_srcs;
   std::map<std::string, Source *>::iterator srcIt = m_sources.begin();
   for ( ; srcIt != m_sources.end(); ++srcIt) {
      if (srcIt->second->getType() == std::string("Diffuse")) {
         DiffuseSource *diffuse_src = 
            dynamic_cast<DiffuseSource *>(srcIt->second);
         diffuse_srcs.push_back(diffuse_src);
      }
   }
   if (diffuse_srcs.size() > 0) {
      EventContainer & eventCont =
         const_cast<EventContainer &>(m_observation.eventCont());
      eventCont.computeEventResponses(diffuse_srcs, sr_radius);
   }
}

void LogLike::findFreeSrcs() const {
   m_freeSrcs.clear();
   std::map<std::string, Source *>::const_iterator src(m_sources.begin());
   for ( ; src != m_sources.end(); ++src) {
      Source::FuncMap & srcFuncs(src->second->getSrcFuncs());
      Source::FuncMap::const_iterator func(srcFuncs.begin());
      bool haveFreePars(false);
      for ( ; func != srcFuncs.end(); ++func) {
         if (func->second->getNumFreeParams() > 0) {
            haveFreePars = true;
         }
      }
      if (haveFreePars) {
         m_freeSrcs.push_back(src->second);
      }
   }
}

} // namespace Likelihood
