/**
 * @file TsMap.cxx
 * @brief Prototype standalone application for producing
 * "test-statistic" maps.
 * @author J. Chiang
 *
 * $Header$
 */

#include <cmath>
#include <cstdlib>
#include <cstring>

#include "fitsio.h"

#include "st_app/AppParGroup.h"
#include "st_app/StApp.h"
#include "st_app/StAppFactory.h"

#include "optimizers/dArg.h"
#include "optimizers/Drmngb.h"
#include "optimizers/Lbfgs.h"
#include "optimizers/Minuit.h"
#include "optimizers/Exception.h"

#include "Likelihood/AppHelpers.h"
#include "Likelihood/LogLike.h"
#include "Likelihood/Util.h"

using namespace Likelihood;

/**
 * @class TsMap
 * @brief Class for encapsulating methods for creating a test-statistic
 * map.
 *
 * @author J. Chiang
 *
 * $Header$
 */
class TsMap : public st_app::StApp {
public:
   TsMap();
   virtual ~TsMap() throw() {
      try {
         delete m_opt;
         delete m_helper;
      } catch (std::exception & eObj) {
         std::cerr << eObj.what() << std::endl;
      } catch (...) {
      }
   }
   virtual void run();
private:
   AppHelpers * m_helper;
   st_app::AppParGroup & m_pars;
   LogLike m_logLike;
   optimizers::Optimizer * m_opt;
   std::vector<double> m_lonValues;
   std::vector<double> m_latValues;
   std::vector< std::vector<double> > m_tsMap;
   PointSource m_testSrc;
   void readSrcModel();
   void readEventData();
   void selectOptimizer();
   void setGrid();
   void computeMap();
   void writeFitsFile(const std::string &filename,
                      std::vector<double> &lon, 
                      std::vector<double> &lat,
                      std::vector< std::vector<double> > &map,
                      const std::string &coordSystem);
   void fitsReportError(FILE *stream, int status);
   void makeDoubleVector(double xmin, double xmax, int nx,
                         std::vector<double> &xVals);
   void setPointSourceSpectrum(PointSource &src);
};

st_app::StAppFactory<TsMap> myAppFactory;

TsMap::TsMap() : st_app::StApp(), m_helper(0), 
                 m_pars(st_app::StApp::getParGroup("TsMap")), m_opt(0) {
   try {
      m_pars.Prompt();
      m_pars.Save();
      m_helper = new AppHelpers(m_pars);
      setPointSourceSpectrum(m_testSrc);
      m_testSrc.setName("testSource");
   } catch (std::exception &eObj) {
      std::cout << eObj.what() << std::endl;
      std::exit(1);
   }  catch (...) {
      std::cerr << "Caught unknown exception in TsMap constructor."
                << std::endl;
      std::exit(1);
   }
}

void TsMap::run() {
   m_helper->setRoi();
   m_helper->readExposureMap();
   readSrcModel();
   readEventData();
   selectOptimizer();
   setGrid();
   computeMap();
   writeFitsFile(m_pars["TS_map_file"], m_lonValues, m_latValues,
                 m_tsMap, m_pars["Coordinate_system"]);
}

void TsMap::readSrcModel() {
   Util::file_ok(m_pars["Source_model_file"]);
   m_logLike.readXml(m_pars["Source_model_file"], m_helper->funcFactory());
}   

void TsMap::readEventData() {
   std::vector<std::string> eventFiles;
   Util::file_ok(m_pars["event_file"]);
   Util::resolve_fits_files(m_pars["event_file"], eventFiles);
   std::vector<std::string>::const_iterator evIt = eventFiles.begin();
   for ( ; evIt != eventFiles.end(); evIt++) {
      Util::file_ok(*evIt);
      m_logLike.getEvents(*evIt, m_pars["event_file_hdu"]);
   }
   m_logLike.computeEventResponses();
}

void TsMap::selectOptimizer() {
   std::string optimizer = m_pars["optimizer"];
   if (optimizer == "LBFGS") {
      m_opt = new optimizers::Lbfgs(m_logLike);
   } else if (optimizer == "MINUIT") {
      m_opt = new optimizers::Minuit(m_logLike);
   } else if (optimizer == "DRMNGB") {
      m_opt = new optimizers::Drmngb(m_logLike);
   }
   if (m_opt == 0) {
      throw std::invalid_argument("Invalid optimizer choice: " + optimizer);
   }
}

void TsMap::setGrid() {
   int nlon = m_pars["Number_of_longitude_points"];
   int nlat = m_pars["Number_of_latitude_points"];
   makeDoubleVector(m_pars["Longitude_min"], m_pars["Longitude_max"], 
                    nlon, m_lonValues);
   makeDoubleVector(m_pars["Latitude_min"], m_pars["Latitude_max"], 
                    nlat, m_latValues);
   m_tsMap.resize(nlat);
   for (int i = 0; i < nlat; i++) {
      m_tsMap.reserve(nlon);
   }
}

void TsMap::computeMap() {
   optimizers::dArg dummy(1.);
      
   int verbosity = m_pars["fit_verbosity"];
   double tol = m_pars["fit_tolerance"];
   m_opt->find_min(verbosity, tol);
   double logLike0 = m_logLike(dummy);
   bool computeExposure(true);
   std::string coordSys = m_pars["Coordinate_system"];

   for (unsigned int jj = 0; jj < m_latValues.size(); jj++) {
      if ( (jj % m_latValues.size()/20) == 0 ) {
         std::cerr << ".";
      }
      for (unsigned int ii = 0; ii < m_lonValues.size(); ii++) {
         if (coordSys == "CEL") {
            m_testSrc.setDir(m_lonValues[ii], m_latValues[jj], 
                             computeExposure, false);
         } else if (coordSys == "GAL") {
            m_testSrc.setGalDir(m_lonValues[ii], m_latValues[jj], 
                                computeExposure, false);
         } else {
            throw std::invalid_argument("Invalid coordinate system: "
                                        + coordSys);
         }
         m_logLike.addSource(&m_testSrc);
         try {
            m_opt->find_min(verbosity, tol);
            m_tsMap[jj].push_back(2.*(m_logLike(dummy) - logLike0));
         } catch (optimizers::Exception &eObj) {
            // Default null value.
            m_tsMap[jj].push_back(0);
         }
         if (verbosity > 0) {
            std::cout << m_lonValues[ii] << "  "
                      << m_latValues[jj] << "  "
                      << m_tsMap[jj][ii] 
                      << std::endl;
         }
         m_logLike.deleteSource(m_testSrc.getName());
      }
   }
   std::cerr << "!" << std::endl;
}

void TsMap::makeDoubleVector(double xmin, double xmax, int nx,
                             std::vector<double> &xVals) {
   xVals.clear();
   xVals.reserve(nx);
   double xstep = (xmax - xmin)/(nx-1);
   for (int i = 0; i < nx; i++) {
      xVals.push_back(xstep*i + xmin);
   }
}

void TsMap::setPointSourceSpectrum(PointSource &src) {
   optimizers::Function * pl = m_helper->funcFactory().create("PowerLaw");
   double parValues[] = {1., -2., 100.};
   std::vector<double> pars(parValues, parValues + 3);
   pl->setParamValues(pars);
   optimizers::Parameter indexParam = pl->getParam("Index");
   indexParam.setBounds(-3.5, -1.);
   pl->setParam(indexParam);
   optimizers::Parameter prefactorParam = pl->getParam("Prefactor");
   prefactorParam.setBounds(1e-10, 1e3);
   prefactorParam.setScale(1e-9);
   pl->setParam(prefactorParam);
   src.setSpectrum(pl);
}

void TsMap::writeFitsFile(const std::string &filename, 
                          std::vector<double> &lon,
                          std::vector<double> &lat,
                          std::vector< std::vector<double> > &map,
                          const std::string &coordSystem) {
   fitsfile *fptr;
   int status = 0;
   
// Always overwrite an existing file.
   remove(filename.c_str());
   fits_create_file(&fptr, filename.c_str(), &status);
   fitsReportError(stderr, status);

   int bitpix = DOUBLE_IMG;
   long naxis = 2;
   long naxes[] = {lon.size(), lat.size()};
   fits_create_img(fptr, bitpix, naxis, naxes, &status);
   fitsReportError(stderr, status);

// Repack exposure into a 1D vector
   std::vector<double> mapVector;
   mapVector.reserve(lon.size()*lat.size());
   for (unsigned int i = 0; i < lon.size(); i++) {
      for (unsigned int j = 0; j < lat.size(); j++) {
         mapVector.push_back(map[i][j]);
      }
   }

// Write the exposure map data.
   long group = 0;
   long firstelem = 1;
   long nelements = lon.size()*lat.size();
   fits_write_img_dbl(fptr, group, firstelem, nelements,
                      &mapVector[0], &status);
   fitsReportError(stderr, status);

// Write some keywords.
   double l0 = lon[0];
   fits_update_key(fptr, TDOUBLE, "CRVAL1", &l0, 
                   "longitude of reference pixel", &status);
   fitsReportError(stderr, status);
   double b0 = lat[0];
   fits_update_key(fptr, TDOUBLE, "CRVAL2", &b0, 
                   "latitude of reference pixel", &status);
   fitsReportError(stderr, status);
   
   double lstep = lon[1] - lon[0];
   fits_update_key(fptr, TDOUBLE, "CDELT1", &lstep, 
                   "longitude step at ref. pixel", &status);
   fitsReportError(stderr, status);
   double bstep = lat[1] - lat[0];
   fits_update_key(fptr, TDOUBLE, "CDELT2", &bstep, 
                   "latitude step at ref. pixel", &status);
   fitsReportError(stderr, status);
   
   float crpix1 = 1.0;
   fits_update_key(fptr, TFLOAT, "CRPIX1", &crpix1, 
                   "reference pixel for longitude coordinate", &status);
   fitsReportError(stderr, status);
   float crpix2 = 1.0;
   fits_update_key(fptr, TFLOAT, "CRPIX2", &crpix2, 
                   "reference pixel for latitude coordinate", &status);
   fitsReportError(stderr, status);
   
   if (coordSystem == "CEL") {
      char *ctype1 = "RA---CAR";
      fits_update_key(fptr, TSTRING, "CTYPE1", ctype1, 
                      "right ascension", &status);
      fitsReportError(stderr, status);
      char *ctype2 = "DEC--CAR";
      fits_update_key(fptr, TSTRING, "CTYPE2", ctype2, 
                      "declination", &status);
      fitsReportError(stderr, status);
   } else if (coordSystem == "GAL") {
      char *ctype1 = "GLON-CAR";
      fits_update_key(fptr, TSTRING, "CTYPE1", ctype1, 
                      "Galactic longitude", &status);
      fitsReportError(stderr, status);
      char *ctype2 = "GLAT-CAR";
      fits_update_key(fptr, TSTRING, "CTYPE2", ctype2, 
                      "Galactic latitude", &status);
      fitsReportError(stderr, status);
   }
   
   fits_close_file(fptr, &status);
   fitsReportError(stderr, status);
   
   return;
}

void TsMap::fitsReportError(FILE *stream, int status) {
   fits_report_error(stream, status);
   if (status != 0) {
      throw std::runtime_error("writeExposureFile: cfitsio error.");
   }
}
