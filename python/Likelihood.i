// -*- mode: c++ -*-
%module Likelihood
%{
#include "astro/SkyDir.h"
#include "optimizers/Parameter.h"
#include "Likelihood/Response.h"
#include "Likelihood/Source.h"
#include "Likelihood/Aeff.h"
#include "Likelihood/DiffuseSource.h"
#include "Likelihood/EventArg.h"
#include "Likelihood/Event.h"
#include "Likelihood/ExposureMap.h"
#include "Likelihood/FitsImage.h"
#include "Likelihood/logSrcModel.h"
#include "Likelihood/Npred.h"
#include "Likelihood/PointSource.h"
#include "Likelihood/Psf.h"
#include "Likelihood/RoiCuts.h"
#include "Likelihood/ScData.h"
#include "Likelihood/SkyDirArg.h"
#include "Likelihood/SkyDirFunction.h"
#include "Likelihood/SourceFactory.h"
#include "Likelihood/SourceModel.h"
#include "Likelihood/SpatialMap.h"
//#include "../Likelihood/SpectrumFactory.h"
#include "Likelihood/SrcArg.h"
#include "Likelihood/Table.h"
#include "Likelihood/TrapQuad.h"
#include "Likelihood/ConstantValue.h"
#include "Likelihood/Exception.h"
#include "../src/logLike_ptsrc.h"
#include <vector>
#include <string>
#include <exception>
using optimizers::Parameter;
%}
%include stl.i
%include ../Likelihood/Exception.h
%include ../Likelihood/Response.h
%include ../Likelihood/Event.h
%include ../Likelihood/Source.h
%include ../Likelihood/SourceModel.h
%include ../Likelihood/Aeff.h
%include ../Likelihood/DiffuseSource.h
%include ../Likelihood/EventArg.h
%include ../Likelihood/ExposureMap.h
%include ../Likelihood/FitsImage.h
%include ../src/logLike_ptsrc.h
%include ../Likelihood/logSrcModel.h
%include ../Likelihood/Npred.h
%include ../Likelihood/PointSource.h
%include ../Likelihood/Psf.h
%include ../Likelihood/RoiCuts.h
%include ../Likelihood/ScData.h
%include ../Likelihood/SkyDirArg.h
%include ../Likelihood/SkyDirFunction.h
%include ../Likelihood/SourceFactory.h
%include ../Likelihood/SpatialMap.h
//%include ../Likelihood/SpectrumFactory.h
%include ../Likelihood/SrcArg.h
%include ../Likelihood/Table.h
%include ../Likelihood/TrapQuad.h
%include ../Likelihood/ConstantValue.h
%template(DoubleVector) std::vector<double>;
%template(DoubleVectorVector) std::vector< std::vector<double> >;
%template(StringVector) std::vector<std::string>;
%extend Likelihood::logLike_ptsrc {
   void print_source_params() {
      std::vector<std::string> srcNames;
      self->getSrcNames(srcNames);
      std::vector<Parameter> parameters;
      for (unsigned int i = 0; i < srcNames.size(); i++) {
         Likelihood::Source *src = self->getSource(srcNames[i]);
         Likelihood::Source::FuncMap srcFuncs = src->getSrcFuncs();
         srcFuncs["Spectrum"]->getParams(parameters);
         std::cout << "\n" << srcNames[i] << ":\n";
         for (unsigned int i = 0; i < parameters.size(); i++)
            std::cout << parameters[i].getName() << ": "
                      << parameters[i].getValue() << std::endl;
         std::cout << "Npred: "
                   << src->Npred() << std::endl;
      }
   }
   void src_param_table() {
      std::vector<std::string> srcNames;
      self->getSrcNames(srcNames);
      std::vector<Parameter> parameters;
      for (unsigned int i = 0; i < srcNames.size(); i++) {
         Likelihood::Source *src = self->getSource(srcNames[i]);
         Likelihood::Source::FuncMap srcFuncs = src->getSrcFuncs();
         srcFuncs["Spectrum"]->getParams(parameters);
         for (unsigned int i = 0; i < parameters.size(); i++)
            std::cout << parameters[i].getValue() << "  ";
         std::cout << src->Npred() << "  ";
         std::cout << srcNames[i] << std::endl;
      }
   }
}
%extend Likelihood::Event {
   double ra() {
      return self->getDir().ra();
   }
   double dec() {
      return self->getDir().dec();
   }
   double scRa() {
      return self->getScDir().ra();
   }
   double scDec() {
      return self->getScDir().dec();
   }
}
%extend Likelihood::FitsImage {
   void fetchCelestialArrays(std::vector<double> &longitude,
                             std::vector<double> &latitude) {
      std::valarray<double> lon;
      std::valarray<double> lat;
      self->fetchCelestialArrays(lon, lat);
      longitude.reserve(lon.size());
      latitude.reserve(lat.size());
      for (unsigned int i = 0; i < lon.size(); i++) {
         longitude.push_back(lon[i]);
         latitude.push_back(lat[i]);
      }
   }
   void fetchImageData(std::vector<double> &image) {
      std::valarray<double> imageArray;
      self->fetchImageData(imageArray);
      image.reserve(imageArray.size());
      for (unsigned int i = 0; i < imageArray.size(); i++)
         image.push_back(imageArray[i]);
   }
}
%extend Likelihood::PointSource {
   void setGalDir(double glon, double glat, bool computeExposure=true) {
      self->setDir( astro::SkyDir(glon, glat, astro::SkyDir::GALACTIC),
                    computeExposure );
   }
}                                                       
