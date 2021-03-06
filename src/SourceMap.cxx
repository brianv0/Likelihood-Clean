/**
 * @file SourceMap.cxx
 * @brief Spatial distribution of a source folded through the instrument
 *        response.
 * @author J. Chiang
 *
 * $Header$
 */

#include <cmath>

#include <algorithm>
#include <deque>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>

#include "st_stream/StreamFormatter.h"

#include "astro/SkyProj.h"

#include "tip/IFileSvc.h"
#include "tip/Image.h"
#include "tip/Table.h"
#include "tip/tip_types.h"

#include "st_facilities/Util.h"

#include "Likelihood/BinnedExposure.h"
#include "Likelihood/BinnedCountsCache.h"
#include "Likelihood/CompositeSource.h"
#include "Likelihood/CountsMapBase.h"
#include "Likelihood/CountsMap.h"
#include "Likelihood/CountsMapHealpix.h"
#include "Likelihood/ConvolveHealpix.h"
#include "Likelihood/AppHelpers.h"
#include "Likelihood/DiffuseSource.h"
#include "Likelihood/Drm.h"
#include "Likelihood/FitUtils.h"
#include "Likelihood/FileUtils.h"
#include "Likelihood/MapBase.h"
#include "Likelihood/MeanPsf.h"
#include "Likelihood/PointSource.h"
#include "Likelihood/PSFUtils.h"
#include "Likelihood/Observation.h"
#include "Likelihood/ResponseFunctions.h"
#include "Likelihood/SkyDirArg.h"
#include "Likelihood/Source.h"
#define ST_DLL_EXPORTS
#include "Likelihood/SourceMap.h"
#undef ST_DLL_EXPORTS
#include "Likelihood/SpatialFunction.h"

#include "Likelihood/WcsMap2.h"
#include "Likelihood/WeightMap.h"
#include "Likelihood/HealpixProjMap.h"


namespace Likelihood { 


void SourceMap::fill_sparse_model(const std::vector<float>& vect,
				  SparseVector<float>& sparse) {
  sparse.resize(vect.size());
  sparse.fill_from_vect(vect);
}

void SourceMap::fill_full_model(const SparseVector<float>& sparse,
				std::vector<float>& vect) {
  sparse.fill_vect(vect);
 }


SourceMap::SourceMap(const Source& src, 
		     const BinnedCountsCache * dataCache,
                     const Observation & observation, 
                     const PsfIntegConfig & psf_config,
		     const Drm* drm,
		     const WeightMap* weights,
		     bool save_model)
   : m_src(&src), 
     m_name(src.getName()),
     m_srcType(src.getType()),
     m_dataCache(dataCache),
     m_observation(observation),
     m_meanPsf(0),
     m_formatter(new st_stream::StreamFormatter("SourceMap", "", 2)),
     m_psf_config(psf_config),
     m_drm(drm),
     m_weights(weights),
     m_mapType(FileUtils::Unknown),
     m_save_model(save_model),
     m_drm_cache(0) {
   
   int status = make_model();
   if ( status != 0 ) {
     throw std::runtime_error("SourceMap construction failed");
   }
   m_drm_cache = new Drm_Cache(m_drm,*this,dataCache->energies());  
}

SourceMap::SourceMap(const std::string & sourceMapsFile,
                     const Source& src, 
		     const BinnedCountsCache * dataCache,
		     const Observation & observation,
		     const WeightMap* weights,
		     const Drm* drm,
		     bool save_model) 
  : m_src(&src),
    m_name(src.getName()),
    m_dataCache(dataCache),
    m_observation(observation),
    m_meanPsf(0),
    m_formatter(new st_stream::StreamFormatter("SourceMap", "", 2)),
    m_weights(weights),
    m_mapType(FileUtils::Unknown),
    m_save_model(save_model),
    m_drm(drm),
    m_psf_config(),
    m_drm_cache(0) {


  int status = readModel(sourceMapsFile);
  if ( status != 0 ) {
    throw std::runtime_error("SourceMap construction failed to read model");
  }
  m_drm_cache = new Drm_Cache(m_drm,*this,m_dataCache->energies());
}


SourceMap::SourceMap(const SourceMap& other)
  :m_src(other.m_src),
   m_name(other.m_name),
   m_filename(other.m_filename),
   m_srcType(other.m_srcType),
   m_dataCache(other.m_dataCache),
   m_observation(other.m_observation),
   m_meanPsf(0), // FIXME
   m_formatter(new st_stream::StreamFormatter("SourceMap", "", 2)),
   m_psf_config(other.m_psf_config),
   m_drm(other.m_drm),
   m_weights(other.m_weights),
   m_save_model(other.m_save_model),
   m_model(other.m_model),
   m_sparseModel(other.m_sparseModel),
   m_mapType(other.m_mapType),
   m_specVals(other.m_specVals),
   m_modelPars(other.m_modelPars),
   m_latchedModelPars(other.m_latchedModelPars),
   m_derivs(other.m_derivs),
   m_npreds(other.m_npreds),
   m_npred_weights(other.m_npred_weights),
   m_drm_cache(other.m_drm_cache != 0 ? other.m_drm_cache->clone() : 0){
}


SourceMap::~SourceMap() {
   delete m_formatter;
   delete m_meanPsf; 
   delete m_drm_cache;
}

float SourceMap::operator[](size_t idx) const {
  return m_mapType == FileUtils::HPX_Sparse ? find_value(idx) : m_model[idx];
}


void SourceMap::sparsify_model(bool clearFull) {
  fill_sparse_model(m_model,m_sparseModel);
  if ( clearFull ) {
    // This deallocates the memory used by the model
    // in C++-11 there is a function shrink_to_fit that we could use.
    std::vector<float> nullVect;
    m_model.swap(nullVect);
  }
}
  

void SourceMap::expand_model(bool clearSparse) {
  fill_full_model(m_sparseModel,m_model);
  if ( clearSparse ) {
    m_sparseModel.clear();
  }
}


void SourceMap::computeNpredArray() {

   bool expanded = false;
   if ( m_mapType == FileUtils::HPX_Sparse && m_model.size() == 0 ) {
     expanded = true;
     expand_model(false);
   }

   if ( m_model.size() == 0 ) {
     // The model was clear, re-make it
     // Note that this call will also call computeNpredArray,
     // so we can return now
     const std::vector<float>& dummy = model(true);
     return;
   }

   const std::vector<double>& energies = m_dataCache->energies();
   

   // If we are making the __weights__ source map,
   // the output npreds is one less than the input map size
   // since we have the energies at the bin edges, but we need the
   // weights integrated over the bins
   size_t ne = energies.size();
   // The number of weights is always number of energy bins
   size_t nw = energies.size() -1 ;
   m_npreds.clear();
   m_npreds.resize(ne);
   m_npred_weights.clear();
   m_npred_weights.resize(nw, std::make_pair<double,double>(0.,0.));

   size_t npix = m_dataCache->num_pixels();

   for (size_t k(0); k < ne; k++) {

      double w_0_sum(0.);
      double w_1_sum(0.);

      for (size_t j(0); j < npix; j++) {
	 size_t indx(k*npix + j);
         size_t indx_0 = k > 0 ? indx  - npix : indx;
	 size_t indx_1 = k < energies.size()-1 ? indx : indx - npix;
	 double addend = m_model.at(indx);
         m_npreds[k] += addend;
	 double w_0_addend = m_weights != 0 ? ( m_weights->model()[indx_0]*addend ) : addend;
	 double w_1_addend = m_weights != 0 ? ( m_weights->model()[indx_1]*addend ) : addend;
	 w_0_sum += w_0_addend;
	 w_1_sum += w_1_addend;
      }      
      double w_0 = m_weights != 0 ? (m_npreds[k] > 0 ? w_0_sum / m_npreds[k] : 0.) : 1.0;
      double w_1 = m_weights != 0 ? (m_npreds[k] > 0 ? w_1_sum / m_npreds[k] : 0.) : 1.0;

      if ( k < nw ) {
	m_npred_weights[k].first = w_0;
      }
      if ( k > 0 ) {
	m_npred_weights[k-1].second = w_1;
      }
   }

   if ( expanded ) {
     m_model.clear();
   }

}


void SourceMap::applyPhasedExposureMap() {
   if (!m_observation.have_phased_expmap()) {
      return;
   }
   const ProjMap * phased_expmap = &(m_observation.phased_expmap());
   const std::vector<Pixel> & pixels = m_dataCache->countsMap().pixels();
   const std::vector<double>&  energies = m_dataCache->energies();
   for (size_t k(0); k < energies.size(); k++) {
      std::vector<Pixel>::const_iterator pixel(pixels.begin());
      for (size_t j(0); pixel != pixels.end(); ++pixel, j++) {
         size_t indx(k*pixels.size() + j);
         m_model.at(indx) *= phased_expmap->operator()(pixel->dir(),
                                                       energies[k]);
      }
   }
}


void SourceMap::setSource(const Source& src) {
  if ( m_src == &src ) {
    return;
  }
  m_src = &src;
  m_specVals.clear();
  m_modelPars.clear();
  m_derivs.clear();
  m_npreds.clear();
  m_npred_weights.clear();  
}

const Drm_Cache* SourceMap::update_drm_cache(const Drm* drm, bool force) {
  bool changed = m_drm != drm;
  m_drm = drm;
  return drm_cache(force || changed);
}

void SourceMap::setSpectralValues(const std::vector<double>& energies,
				    bool latch_params ) {
  if ( m_src == 0 ) return;
  FitUtils::extractSpectralVals(*m_src,energies,m_specVals);
  m_modelPars.clear();
  m_src->spectrum().getParamValues(m_modelPars);
  if ( latch_params ) {
    m_latchedModelPars.resize(m_modelPars.size());
    std::copy(m_modelPars.begin(),m_modelPars.end(),m_latchedModelPars.begin());
  }
}


void SourceMap::setSpectralDerivs(const std::vector<double>& energies,
				  const std::vector<std::string>& paramNames) {
  if ( m_src == 0 ) return;
  FitUtils::extractSpectralDerivs(*m_src,energies,paramNames,m_derivs);
}


bool SourceMap::spectrum_changed() const {
  if ( m_src == 0 ) return true;
  std::vector<double> parValues;
  m_src->spectrum().getParamValues(parValues);
  if ( parValues.size() != m_latchedModelPars.size() ) {
    return true;
  }
  for (size_t j(0); j < parValues.size(); j++) {
    if (parValues.at(j) != m_latchedModelPars.at(j) ) {
      return true;
    }
  }
  return false;
}


const std::vector<float> & SourceMap::model(bool force) {
  if ( m_model.size() == 0 || force ) {
    if ( m_filename.size() > 0 ) {        
      readModel(m_filename);
    } else {
      int status = make_model();
      if ( status != 0 ) {
	throw std::runtime_error("SourceMap model");
      }
    }
  }
  return m_model;
}
  

const std::vector<double> & SourceMap::specVals(bool force) {
  if ( m_specVals.size() == 0 || force ) {
    setSpectralValues(m_dataCache->energies());
  }
  return m_specVals;
}


const std::vector<std::vector<double> >& SourceMap::specDerivs(const std::vector<std::string>& paramNames, bool force) {
  if ( m_specVals.size() == 0 || force ) {
    setSpectralDerivs(m_dataCache->energies(),paramNames);
  }
  return m_derivs;
}


const std::vector<double> & SourceMap::npreds(bool force) {
  if ( m_npreds.size() == 0 || force ) {
    computeNpredArray();
  }
  return m_npreds;
}
  
const std::vector<std::pair<double,double> > & SourceMap::npred_weights(bool force) {
  if ( m_npred_weights.size() == 0 || force ) {
    computeNpredArray();
  }
  return m_npred_weights;
}  

const Drm_Cache* SourceMap::drm_cache(bool force) {
  if ( m_drm_cache == 0) {
    m_drm_cache = new Drm_Cache(m_drm,*this,m_dataCache->energies());  
  } else if ( force ) {
    m_drm_cache->update(m_drm,*this,m_dataCache->energies());  
  }
  return m_drm_cache;
}
 
void SourceMap::addToVector(std::vector<float>& vect, bool includeSpec) {
  switch ( m_mapType ) {
  case FileUtils::HPX_Sparse:
    return addToVector_sparse(vect,includeSpec);
    break;
  case FileUtils::WCS:
  case FileUtils::HPX_AllSky:
  case FileUtils::HPX_Partial:
  default:
    return addToVector_full(vect,includeSpec);
  }
}
   
void SourceMap::subtractFromVector(std::vector<float>& vect, bool includeSpec){
  switch ( m_mapType ) {
  case FileUtils::HPX_Sparse:
    return subtractFromVector_sparse(vect,includeSpec);
    break;
  case FileUtils::WCS:
  case FileUtils::HPX_AllSky:
  case FileUtils::HPX_Partial:
  default:
    return subtractFromVector_full(vect,includeSpec);
  }
}

double SourceMap::summed_counts(size_t kmin, size_t kmax,
				bool use_edisp,
				bool use_weighted) {

  if ( m_drm_cache == 0 ) {
    throw std::runtime_error("SourceMap::summed_counts: no Drm_Cache");
  }  

  const std::vector<double>& counts_spec = use_edisp ? 
    ( use_weighted ? m_drm_cache->meas_counts_wt() : m_drm_cache->meas_counts() ) :
    ( use_weighted ? m_drm_cache->true_counts_wt() : m_drm_cache->true_counts() );
     
  double value(0.);
  for (size_t k(kmin); k < kmax; k++) {    
    value += counts_spec[k];
  }
  return value;    
}

void SourceMap::setImage(const std::vector<float>& model) {
  if(model.size() != m_model.size())
    throw std::runtime_error("Wrong size for input model map.");

  m_model = model;
  m_filename.clear();
  applyPhasedExposureMap();
  computeNpredArray();
}

void SourceMap::setWeights(const WeightMap* weights) {
  m_weights = weights;
  computeNpredArray();
}


size_t SourceMap::memory_size() const {
  size_t retVal(0);
  retVal += sizeof(*this);
  retVal += m_name.capacity();
  retVal += m_filename.capacity();
  retVal += m_srcType.capacity();
  retVal += sizeof(*m_formatter);
  retVal += sizeof(float)*m_model.capacity();
  retVal += sizeof(std::pair<size_t,float>)*m_sparseModel.capacity();
  retVal += sizeof(double)*m_modelPars.capacity();
  retVal += sizeof(double)*m_npreds.capacity();
  retVal += sizeof(std::pair<double,double>)*m_npred_weights.capacity();
  if ( m_drm_cache != 0 ) {
    retVal += m_drm_cache->memory_size();
  }
  return retVal;
}


void SourceMap::test_sparse(const std::string& prefix) const {
  for ( SparseVector<float>::const_iterator itr = m_sparseModel.begin();
	itr != m_sparseModel.end(); itr++ ) {
    if ( itr->first >= m_sparseModel.size() ) {
      std::cout << prefix << " " << itr->first << ' ' << m_sparseModel.size() << ' ' << itr->second << std::endl;
    }
  }
}


int SourceMap::readModel(const std::string& filename) {
  m_model.clear();
  m_filename = filename;
  
  m_specVals.clear();
  m_modelPars.clear();
  m_derivs.clear();
  m_npreds.clear();
  m_npred_weights.clear();

  int status(0);
  switch ( m_dataCache->countsMap().projection().method()  ) {
  case astro::ProjBase::WCS:
    status = readImage(m_filename);
    break;
  case astro::ProjBase::HEALPIX:
    status = readTable_healpix(m_filename);
    break;
  default:
    break;
  }
  if ( status != 0 ) {
    std::string errMsg("SourceMap failed to read source map: ");
    errMsg += m_filename;
    errMsg += ".  To match data file: ";
    errMsg += m_dataCache->countsMap().filename();
    throw std::runtime_error(errMsg);
  }

  // FIXME, we could be more efficient about this
  if ( m_mapType == FileUtils::HPX_Sparse ) {
    expand_model(false);
  }

  applyPhasedExposureMap();
  computeNpredArray();
  setSpectralValues(m_dataCache->energies());

  // FIXME, we could be more efficient about this
  if ( m_mapType == FileUtils::HPX_Sparse ) {
    m_model.clear();
  }

  return status;
}

int SourceMap::readImage(const std::string& sourceMapsFile) {
  m_mapType = FileUtils::get_src_map_type(sourceMapsFile,m_name);
  FileUtils::read_fits_image_to_float_vector(sourceMapsFile,m_name,m_model);
  return 0;
}

int SourceMap::readTable_healpix(const std::string& sourceMapsFile) {
  m_mapType = FileUtils::get_src_map_type(sourceMapsFile,m_name);
  int status(0);
  switch (m_mapType) {
  case FileUtils::HPX_AllSky:
  case FileUtils::HPX_Partial:
    // In either of these two cases we simple read the vector.  
    // If this is a partial-sky mapping, the projection will 
    // take care of doing the remapping
    status = FileUtils::read_healpix_table_to_float_vector(sourceMapsFile,m_name,m_model);
    break;
  case FileUtils::HPX_Sparse:
    // In this case we read the map.
    /// FIXME, we should have a better way of getting this...
    m_sparseModel.resize( m_dataCache->source_map_size() );
    status = FileUtils::read_healpix_table_to_sparse_vector(sourceMapsFile,m_name,m_sparseModel);
    break;
  default:
    // Either unknown or WCS based.  This is an error in either case.
    return -1;
  }
  
  if ( status != 0 ) return status;


  return 0;

}


int SourceMap::make_model() {
  if ( m_src == 0 ) return -1;
  
  m_filename.clear();
  m_model.clear();
  m_specVals.clear();
  m_modelPars.clear();
  m_derivs.clear();
  m_npreds.clear();
  m_npred_weights.clear();

  int status(0);
  switch ( m_src->srcType() ) {
  case Source::Diffuse:
    status = PSFUtils::makeDiffuseMap(static_cast<const DiffuseSource&>(*m_src), m_dataCache->countsMap(), 
				      m_observation.meanpsf(), m_observation.bexpmap(),
				      m_psf_config, *m_formatter, m_model, m_mapType);
    break;
  case Source::Point:
    m_meanPsf = m_psf_config.use_single_psf() ? 0 : PSFUtils::build_psf(*m_src, m_dataCache->countsMap(),m_observation);
    status =  PSFUtils::makePointSourceMap(static_cast<const PointSource&>(*m_src), m_dataCache->countsMap(), 
					   m_psf_config, m_meanPsf==0 ? m_observation.meanpsf() : *m_meanPsf, 
					   *m_formatter, m_model, m_mapType);
    break;
  case Source::Composite:
    status =  PSFUtils::makeCompositeMap(static_cast<const CompositeSource&>(*m_src), *m_dataCache, 
					 m_filename, m_drm,
					 *m_formatter, m_model, m_mapType);    
    break;
  default:
    throw std::runtime_error("Unrecognized source type");
  }
  
  if ( status != 0 ) { 
    return status;
  }

  
  applyPhasedExposureMap();
  computeNpredArray();
  setSpectralValues(m_dataCache->energies());

  // FIXME, we could be more efficient about this
  if ( m_mapType == FileUtils::HPX_Sparse ) {
    sparsify_model();
  }

  return status;
}

void SourceMap::addToVector_full(std::vector<float>& vect, bool includeSpec) const {
  const std::vector<float>& m = model();
  if ( vect.size() != m.size() ) {
    std::cout << "SourceMap::addToVector_full " << vect.size() << ' ' << m.size() << std::endl;
    throw std::runtime_error("SourceMap::addToVector_full model size != vector size");
  }
  size_t ne =  m_dataCache->num_energies();
  size_t npix = m_dataCache->num_pixels();
  if ( includeSpec && m_specVals.size() != ne ) {
    throw std::runtime_error("SourceMap::addToVector_full spectrum size != number of energy layers");
  }
  std::vector<float>::const_iterator itr_in = m.begin();
  std::vector<float>::iterator itr_out = vect.begin();
  for ( size_t ie(0); ie < ne; ie++ ) {
    double factor = includeSpec ? m_specVals[ie] : 1.;
    for ( size_t ipix(0); ipix < npix; ipix++, itr_in++, itr_out++ ) {
      *itr_out += *itr_in * factor;
    }
  }
}
  
void SourceMap::addToVector_sparse(std::vector<float>& vect, bool includeSpec) const {
  if ( vect.size() != m_sparseModel.size() ) {
    throw std::runtime_error("SourceMap::addToVector_sparse model size != vector size");
  }
  size_t ne = m_dataCache->num_energies();
  size_t npix = m_dataCache->num_pixels();
  if ( includeSpec && m_specVals.size() != ne ) {
    throw std::runtime_error("SourceMap::addToVector_sparse spectrum size != number of energy layers");
  }
  size_t idx(0);
  for ( SparseVector<float>::const_iterator itr = m_sparseModel.begin(); 
	itr != m_sparseModel.end(); itr++, idx++) {
    int ie = includeSpec ? ( itr->first / npix ) : -1;    
    double factor = ie >= 0 ? m_specVals[ie] : 1.;
    vect[itr->first] += itr->second * factor;
  }
}

void SourceMap::subtractFromVector_full(std::vector<float>& vect, bool includeSpec) const {
  if ( vect.size() != m_model.size() ) {
    throw std::runtime_error("SourceMap::subtractFromVector_full model size != vector size");
  }
  size_t ne = m_dataCache->num_energies();
  size_t npix = m_dataCache->num_pixels();
  if ( includeSpec && m_specVals.size() != ne ) {
    throw std::runtime_error("SourceMap::subtractFromVector_full spectrum size != number of energy layers");
  }
  std::vector<float>::const_iterator itr_in = m_model.begin();
  std::vector<float>::iterator itr_out = vect.begin();
  for ( size_t ie(0); ie < ne; ie++ ) {
    double factor = includeSpec ? m_specVals[ie] : 1.;
    for ( size_t ipix(0); ipix < npix; ipix++, itr_in++, itr_out++ ) {
      *itr_out -= *itr_in * factor;
    }
  }
}
 

void SourceMap::subtractFromVector_sparse(std::vector<float>& vect, bool includeSpec) const {
  if ( vect.size() != m_sparseModel.size() ) {
    throw std::runtime_error("SourceMap::subtractFromVector_sparse model size != vector size");
  }
  size_t ne =  m_dataCache->num_energies();
  size_t npix = m_dataCache->num_pixels();
  if ( includeSpec && m_specVals.size() != ne ) {
    throw std::runtime_error("SourceMap::subtractFromVector_sparse spectrum size != number of energy layers");
  }
  for ( SparseVector<float>::const_iterator itr = m_sparseModel.begin(); 
	itr != m_sparseModel.end(); itr++ ) {
    int ie = includeSpec ? ( itr->first / npix ) : -1;
    double factor = ie >= 0 ? m_specVals[ie] : 1.;
    vect[itr->first] -= itr->second * factor;
  }
} 

} // Likelihood
