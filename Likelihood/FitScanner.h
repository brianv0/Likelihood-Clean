/**
 * @file FitScanner.h
 * @author Eric Charles
 *
 * A class to provide functionality that involves
 * scanning a test source over location or energy bins 
 * or normalization values.
 *
 * This is useful for making TSMaps, SEDs or likelihood v. flux maps 
 *
 * This class uses a number of approximations and shortcuts for speed.
 * 
 *   1) This only runs on binned data.
 *   2) Only the source normalizations are floated, not the other spectral 
 *  parameters.
 *   3) The fitting is done using Newton's method.  The normalizations 
 *  are not allowed to be negative, but this is simply enforced by not
 *  allowing the step between interations to be more negative than the 
 *  current value of the parameter.
 *   4) The gradiant (first derivatives) and Hessian (second derivatives) of the
 *  log-likelihood are calculated analytically.
 *   5) The convergence criterium is calculated using the estimated vertical 
 *  distance to the mininum.  This is computed as the inner product of the 
 *  current step sizes with the gradient vector, which is likely to be an 
 *  overestimate of the actual vertical distance to the minimum (by up to a 
 *  factor of 2)
 *   6) The expected counts model for the test source is computed at new locations
 *  by computing the counts model once at the center of the region of interest, 
 *  then translating that model by moving it a fixed number of pixels to each 
 *  new test location.   This will result in some projection inaccuracies.
 *
 *
 *  This file also contains two helper classes used by FitScanner:  
 *    TestSourceModelCache -> Used to cache the model image of the test source
 *    FitScanCache -> Used to cache the actual counts data and models for efficient fitting
 *
 * $Header$
 */

#ifndef Likelihood_FitScanner_h
#define Likelihood_FitScanner_h

// stl includes
#include <string>
#include <vector>
#include <map>

// CLHEP include

#include "CLHEP/Matrix/SymMatrix.h"
#include "CLHEP/Matrix/Vector.h"

// Fermi includes
#include "astro/SkyDir.h"



// forward declarations
namespace astro {
  class SkyDir;
  //  class ProjBase;
  class SkyProj;
  // class HealpixProj;
}

namespace evtbin {
  class Binner;
}

namespace tip {
  class Header;
}

namespace optimizers{
  class Optimizer;
}


// This class lives in the Likelihood namespace
namespace Likelihood {

  /**
   * @class FitScanner
   * 
   */
   
  class BinnedLikelihood;
  class LogLike;
  class HistND;
  class Source;
  class AppHelpers;
  class CountsMapBase;  


  /* A utility class to cache the image the predicted counts map for the
     test source.  
     
     Simply moving the image around is much faster that recomputing it for each 
     point in the TS-map grid. 

     For small grids this is not to bad of an approximation.

     FIXME, add details on size of errors expected.
   */
  class TestSourceModelCache {

  public:

    /* Build from a BinnedLikelihood and a Source */
    TestSourceModelCache(const BinnedLikelihood& logLike,
			 const Source& source);

    /* D'tor, does nothing */
    ~TestSourceModelCache(){};
    
    /* Translate the cached map to a new location 

       newRef      : The new direction of the center of the model image
       out_model   : Filled with the values of the new model image

       returns 0 for success, error code for failure
     */
    int translateMap(const astro::SkyDir& newRef,
		     std::vector<float>& out_model) const;

    /* Write the current cached map to a FITS image
       
       fits_file    : Name of the fits file in question
       ext_name     : Nmae of the iamge extension
    */
    void writeTestSourceToFitsImage(const std::string& fits_file,
				    const std::string& ext_name) const; 
    
  protected:

    /* Translate the map using WCS projection by dx and dy pixels 

       dx          : Number of pixels offset in X
       dy          : Number of pixels offset in Y
       out_model   : Filled with the values of the new model image     

       returns 0 for success, error code for failure
    */ 
    int translateMap_Wcs(double dx, double dy, std::vector<float>& out_model) const;

    /* Translate the map using HEALPix projection by d_theta and d_phi degrees */
    int translateMap_Healpix(double d_theta, double d_phi, std::vector<float>& out_model) const {
      // FIXME, implement this
      return -1;
    }

  private:
          
    /* The reference image */
    std::vector<float> m_refModel;
    /* The projection used for both the reference and scanned images*/
    // const astro::ProjBase& m_proj;
    const astro::SkyProj& m_proj;
    /* The reference direction */
    const astro::SkyDir& m_refDir;

    /* The location of the reference pixel in image coordinates */
    std::pair<double,double> m_refPixel;

    /* Number of bins in x,y and energy */
    size_t m_nx;
    size_t m_ny;
    size_t m_ne;

    /* The current image */
    mutable std::vector<float> m_currentModel;

  };


  /* A utility class to store and apply a multivariate prior on fit parameters.

     The prior is expressed as a set of best fit values, and the covariance matrix between them.
     This corresponds to a set of Gaussian-distributed errors about the best-fit estimates.     
     
     In partical terms, the prior is applied by adding an addition term to the 
     loglikelihood, as therefore also to the gradiant and Hessian during the fitting.
   */
  class FitScanMVPrior {

  public:

    FitScanMVPrior(const CLHEP::HepVector& centralVals,
		   const CLHEP::HepSymMatrix& covariance,
		   const std::vector<bool>& constrainPars,
		   bool includeTestSource){
      update(centralVals,covariance,constrainPars,includeTestSource);
      latchReducedMatrix();
    }
    
    /* D'tor, does nothing */
    ~FitScanMVPrior() {;}

    // Update the cached values
    void update(const CLHEP::HepVector& centralVals,
		const CLHEP::HepSymMatrix& covariance,
		const std::vector<bool>& constrainPars,
		bool includeTestSource);

    // Calculate the contribution to the negative log-likelihood
    void negativeLogLikelihood(const CLHEP::HepVector& params, 
			       double& logLike) const;

    // Calculate the contribution to the gradient of the log-likelihood
    void gradient(const CLHEP::HepVector& params, 
		  CLHEP::HepVector& grad) const;

    // Calculate the contribution to the gradient 2nd derivativs of the log-likelihood.
    // By construction this is actually just the constant Hessian matrix
    inline const CLHEP::HepSymMatrix& hessian() const { 
      return m_hessian; 
    }

    // These are the central values that we are constraining towards
    inline const CLHEP::HepVector& centralVals() const { return m_centralVals; }    
    // This covariance matrix that gives the strength of the constraints
    inline const CLHEP::HepSymMatrix& covariance() const { return m_covariance; }
    /* This vector is the size of _all_ of the cached parameters,
       it tells use which should be constrained (all the other are FIXED).
       To have a free parameter just put in a very weak constraint */
    inline const std::vector<bool> constrainPars() const { return m_constrainPars; }
    /* Is the test source include in this constraint */
    inline bool includeTestSource() const { return m_includeTestSource; }

  protected:
    
    int latchReducedMatrix();
    
  private:
    
    CLHEP::HepVector m_centralVals;
    CLHEP::HepSymMatrix m_covariance;
    std::vector<bool> m_constrainPars;
    bool m_includeTestSource;

    CLHEP::HepSymMatrix m_hessian;
  };


  /* A utility class to extract the data need for fitting from a BinnedLikelihood object     
     and perform scans using that data, model and a test source.

     The extracted data are stored as series of std::vector<float>, where each vector
     corresponds to either a data map, or a predicted counts model for a specific model component.

     The fits are done with respect to the original source models extracted from the 
     BinnedLikelihood object.  The returned fit parameters are scale factors that need to 
     be applied to the normalization parameters in the XML model used by gtlike, with the
     exception of the test source, where the fit parameter is the normalization of the 
     test source.

     The FitScanCache::fitCurrent() function will fit the currently cached parameters and models.
     
     setTestSource() and shiftTestSource() can be used to move the location of the test source

     addTestSourceToCurrent() and removeTestSourceFromCurrent() can be used to control if the 
     test source is included in the fit

     refactorModel() can be used to fix and freeze other sources in the ROI.  Note that refactorModel()
     operates w.r.t. the baseline model of the region (i.e., the one that was copied in from the 
     BinnedLikelihood object)

     setEnergyBin() sets flags to only do the fit for a single energy bin

     scanNormalization() will do a series of fits, scanning the normalization parameter and refitting
     any other free parameters at each scan point (i.e., a "profile" likelihood)
     
  */  
  class FitScanCache {
  public:

    /* Build from a BinnedLikelihood object
       
       This copies over all the needed data from the BinnedLikelihood object,
       but retains a reference to the object for access to the Observation and 
       CountsMap objects.
     */
    FitScanCache(BinnedLikelihood& binnedLike,
		 const std::string& testSourceName,
		 double tol, int maxIter,
		 bool useReduced = false);

    /* D'tor */
    ~FitScanCache();

    /* Refactor the current model, fixing or freeing sources, and changing normalizations 
       
       freeSource     : Marks which source are free
       parScales      : Current values of the normalization parameters
       include_test   : If true, includes test source in fit model
     */
    void refactorModel(const std::vector<bool>& freeSources, 
		       const std::vector<float>& pars_scales,
		       bool include_test);

    /* Extract the scaling values for the normalization parameters for the current fit */
    void getParScales(std::vector<float>& pars_scales);
    
    /* Set the cache to only do the fit over a single energy bin */ 
    void setEnergyBin(int energyBin);
    
    /* Update the model of the test source.
       This version recomputes the SourceMap and somewhat more expensive (and accurate)
       than shiftTestSource
    */
    void setTestSource(Source& aSrc);

    /* Update the model of the test source
       This version shift the SourceMap with respect to a precomputed version
       and in much less expensive, but not quite as accurate 
     */
    int shiftTestSource(const TestSourceModelCache& modelCache,
			const astro::SkyDir& newDir);

    /* Set the cache to add in the test source with a specify normalization value */
    void addTestSourceToCurrent(double initNorm);

    /* Set the cache to remove the test source from the fit */
    void removeTestSourceFromCurrent();

    /* Set the prior */
    void buildPriorsFromExternal(const CLHEP::HepVector& centralVals,
				 const CLHEP::HepSymMatrix& covariance,
				 const std::vector<bool>& constrainPars);

    /* Set the prior from the current fit*/
    void buildPriorsFromCurrent(const std::vector<bool>& constrainPars,
				double covScaleFactor);
 
    /* Fit the currently cached values using Newton's method */
    int fitCurrent(bool usePrior = false, int verbose=0);

    /* Calculate the log-likelihood for the currently cached values */
    int calculateLoglikeCurrent(double& logLike);

    /* Scan the log likelihood versus the normalizaton of the test source 
       
       nnorm      : Number of scan points
       normSigma  : Number of sigma +/- to scan over
       pos_errs   : Positive side errors on the scan
       neg_errs   : Negative side errors on the scan
       norms      : Filled with normalization values used in the scan
       logLikes   : Filled with the corresponding log likelihood values     
     */
    int scanNormalization(int nnorm, double normSigma,
			  double posErr, double negErr,
			  std::vector<double>& norms,
			  std::vector<double>& logLikes);

    /* Estimate the uncertainty using quadratic equation.

       This accounts for both the normal quadratic dependence,
       and also the linear dependence that you get when 
       the signal is near zero.
       
       deltaLogLike  : log-likelihood value we are solving for
       pos_errs      : Positive side errors on the scan
       neg_errs      : Negative side errors on the scan
    */
    int signalUncertainty_quad(double deltaLogLike,
			       double& posErr,
			       double& negErr);

    // access --------------------------------------------------------

    // Information about the baseline model
    inline const BinnedLikelihood& binnedLikelihood() const { return m_binnedLike; }
    inline const std::string& testSourceName() const { return m_testSourceName; }
    inline size_t npix() const { return m_npix; }
    inline size_t nebins() const { return m_nebins; }
    inline size_t nBkgModel() const { return m_allModels.size(); }
    inline bool useReduced() const { return m_useReduced; }

    inline const double& loglike_ref() const { return m_loglike_ref; }    

    // Information about the currently cached model
    inline size_t nFreeCurrent() const { return m_currentModels.size(); }
    inline int testSourceIndex() const { return m_currentTestSourceIndex; }

    // Information about the current fit
    inline const CLHEP::HepVector& currentPars() const { return m_currentPars; }
    inline const CLHEP::HepSymMatrix& currentCov() const { return m_currentCov; }
    inline const std::vector<float>& currentModel() const { return m_currentBestModel; }
    inline const double& currentLogLike() const { return m_currentLogLike; }
    inline const double& currentEDM() const { return m_currentEDM; }    
    inline int currentEnergyBin() const { return m_currentEnergyBin; }
    	

  protected:

    void reduceModels();

  private:

    // A reference to the BinnedLikelihood object we extracted everything from
    BinnedLikelihood& m_binnedLike;

    // The name of the test source
    const std::string m_testSourceName;
    
    // Fit tolerance
    double m_tol;    
    // maximum number of interactions
    int m_maxIter;

    // number of energy bins in the counts map
    size_t m_nebins;
    // number of pixels in the counts map
    size_t m_npix;

    // reference to counts data
    const std::vector<float>& m_data;
    // master list of all the models, except for the test source
    std::vector<std::vector<float> > m_allModels;
    // master version of the sum of the fixed model components
    std::vector<float> m_allFixed;
    // set of refrence values of all the paramters
    std::vector<float> m_refValues;
    // current test source model map
    std::vector<float> m_targetModel;

    // specific for sparse model fitting
					       
    // Use the reduced vectors?
    bool m_useReduced;
    // Reduced data vector
    std::vector<float> m_dataRed;
    // Indices of non-zero bins
    std::vector<int> m_nonZeroBins;
    // Indices of last bin in each energy bin
    std::vector<int> m_energyBinStopIdxs;
    // master list of all the reduced models, except for the test source
    std::vector<std::vector<float> > m_allRedModels;
    // master version of the reduced models sum of the fixed model components
    std::vector<float> m_allRedFixed;
    // current test source reduced model map
    std::vector<float> m_targetRedModel;


    // log-likelihood for the reference fit
    double m_loglike_ref;

    // this is the list of models for the current fit
    // it may or may not include the test source model
    std::vector<const std::vector<float>* > m_currentModels;
    // this is the fixed component for the current fit
    std::vector<float> m_currentFixed;
    // this is the set of refrence values for the paremeters in the current fit
    std::vector<float> m_currentRefValues;
    // these are the indices of the components in the master list
    std::vector<int> m_currentSourceIndices;
    // this is the index of the test source in the current fit
    int m_currentTestSourceIndex;
    // these are the initial parameters for the current fit
    CLHEP::HepVector m_initPars;
    // these are the current parameters for the current fit
    CLHEP::HepVector m_currentPars;
    // this is the current covarience martix for the current fit
    CLHEP::HepSymMatrix m_currentCov;
    // this is the current gradient for the current fit
    CLHEP::HepVector m_currentGrad;
    // these are the prior to use for the current fit (
    FitScanMVPrior* m_prior_test;
    FitScanMVPrior* m_prior_bkg;

    // this is the best-fit model for the current fit
    std::vector<float> m_currentBestModel;
    // this is the log-likelihood for the current fit
    double m_currentLogLike;
    // this is the estimated distance to minimum for the current fit
    double m_currentEDM;

    // this is the energy bin for the current fit
    // -1 means fit all ranges
    int m_currentEnergyBin;

    // this are the overall bin ranges for the current fit
    size_t m_firstBin;
    size_t m_lastBin;
    
  };



  /* A class to perform a series of related fits in a single ROI with a test source 

     This class can scan over:
     
     1) The test source location (a so-called TSMap)
     2) Energy bins (i.e., to make an SED)
     3) The normalization of the test source (i.e., to make to so-called "Castro" plot)

  */

  class FitScanner {

  public:

    /* Utility function to build an evtbin::Binner object with the energy bins */
    static evtbin::Binner* buildEnergyBinner(const std::vector<double>& energies);

  public:
    
    // C'tor from WCS grid of directions
    FitScanner(LogLike& logLike,
	       optimizers::Optimizer& optimizer,
	       const astro::SkyProj& proj,
	       int nx, int ny);

    // C'tor from HEALPix region set of directions
    /*
    FitScanner(LogLike& logLike,
	       optimizers::Optimizer& optimizer,
	       const astro::HealpixProj& proj,
	       const std::string& region);
    */

    // D'tor, does cleanup
    virtual ~FitScanner() throw();
    
    /* Build a TS map.
       This scans over the directions and calculates the Test Statistics w.r.t. the null 
       hypothesis for each direction 

       This actually just calls run_tscube with doSED and doNorm set to false.

       returns 0 for success, or a error code
     */
    int run_tsmap(double tol=1e-3, 
		  int tolType = 0, 
		  int maxIter = 30, 
		  bool remakeTestSource = false);

    /* Build a TS cube.
       For each point in a TS Map this also calculate the spectrum as a function of energy
       and can also scan over the normalization

       doSED         : Compute the energy bin-by-bin fits
       nNorm         : Number of points in the likelihood v. normalization scan
       covScale      : Scale factor to apply to broadband fitting cov. matrix in bin-by-bin fits ( < 0 -> fixed )
       normSigma     : Number of sigma to use for the scan range 
       tol           : Critetia for fit convergence (estimated vertical distance to min < tol )
       maxIter       : Maximum number of iterations for the Newton's method fitter
       tolType       : Absoulte (0) or relative (1) criteria for convergence
       remakeTestSource : If true, recomputes the test source image (otherwise just shifts it)
       ST_scan_level : Level to which to do ST-based fitting (for testing)

       returns 0 for success, or a error code

       throws exceptions if some inconsistency in the models or data is detected.
     */
    int run_tscube(bool doSED=true, int nNorm = 10, double normSigma = 5.0, 
		   double covScale = -1.0, double tol = 1e-3, int maxIter = 30, int tolType = 0, 
		   bool remakeTestSource = false,
		   int ST_scan_level = 0);

    /* Write the stored data to a FITS file */
    int writeFitsFile(const std::string& fitsFile,
		      const std::string& creator,
		      const std::string& fits_template,
		      bool copyGTIs=false) const;
      

    // Access Functions -----------

    // Likelihood object
    inline const LogLike* logLike() const { return m_logLike; }

    /* Optimizer used to do full fitting 

       In practice this will not be used unless it is specifcially requested.
     */
    inline const optimizers::Optimizer* optimizer() const { return m_opt; }

    // The projection used to bin the data
    // inline const astro::ProjBase* proj() const { return m_proj; }
    inline const astro::SkyProj* proj() const { return m_proj; }

    // The currect direction for the test source
    inline const astro::SkyDir& testSourceDir() const { return m_testSourceDir; }

    /* The test source

       In practice this will not be moved during the scanning unless 
       full-fitting is requested.  

       Instead, the test source will be used to calculate a reference image, 
       and the image will then be moved by offsetting it by a the correct number of pixels
     */
    inline const Source* testSource() const { return m_testSource; }    

    // Name of the test source (for bookkeeping)
    inline const std::string& testSourceName() const { return m_testSourceName; }

    // The number of pixels we are scanning over
    int nPixels() const;

    // The number of energy bins we are scanning over
    int nEBins() const;
    
    // The number of normalization points we are scanning over
    int nNorms() const;


    // Modifiers (use with extreme caution) -----------

    /* Use a powerlaw point source */ 
    int setPowerlawPointTestSource(AppHelpers& helper, double index=2.0);    

    /* Use a source from the model */
    int setTestSourceByName(const std::string& sourceName);

  protected:

    /* This adds the test source to the source model */
    int addTestSourceToModel();

    /* This removes the test source from the source model */
    void removeTestSourceFromModel();

    /* Set the direction of the test source, based on the loop parameters */
    int setTestSourceDir(int ix, int iy);

    /* This does the baseline fit
       i.e., the fit without the test source */
    int baselineFit(double tol = 1e-3, int tolType = 0);

    /* This does the baseline fit with Newton's Method,
       for the normalization parameters only */
    int baselineFit_Newton(double tol = 1e-3, int maxIter = 30);

    /* This does the broadband fit
       i.e., the fit with the source across the entire energy range */
    int fitTestSourceBroadband(double tol = 1e-3, int tolType = 0);   

    /* This does the sed fitting with Newton's method,                                                       
       for the normalization paramters only */
    int sed_binned_newton(int nnorm, double normSigma,
			  double constrainScale,
			  std::vector<double>& norm_mles,
			  std::vector<double>& pos_errs,
			  std::vector<double>& neg_errs,
			  std::vector<double>& logLike_mles,
			  std::vector<std::vector<double> >& norms,
			  std::vector<std::vector<double> >& logLikes);

    /* Build and cache an image of the test source */
    int buildTestModelCache();
    
    /* Build an n-dimensional histogram based on the loop parameters */
    HistND* buildHist(const std::string& name, 
		      bool do_pix = true,
		      bool do_energy = false,
		      bool do_norm = false);        

    /* Write a histogram as a FITS image */
    int writeFitsImage(const std::string& fitsFile,
		       const std::string& extName,
		       const HistND& hist) const;
    
    /* Write a histogram as a FITS table */
    int writeFitsTable_byPixel(const std::string& fitsFile,
			       const std::string& extName,
			       const std::vector<std::pair<std::string,std::pair<HistND*,std::string> > >& colData) const;

    /* write the energy bins */
    int writeFits_EnergyBins(const std::string& fitsFile) const;

    /* write the Good time intervals */
    int writeFits_GTIs(const std::string& fitsFile) const;    

    /* Convert the dimension string to the format expected by FITS */
    bool convertDimString(const std::string& inString,
			  std::string& outString, 
			  bool do_pix = false,
			  bool do_energy = true,
			  bool do_norm = true) const;

    /* set the TDIM keyword */
    void setDimKeyword(tip::Header& header,
		       int icol,
		       const std::string& dimString) const;

  private:

    // The log-likelihood object (also the source model)
    LogLike* m_logLike;
    // The optimizer
    optimizers::Optimizer* m_opt;    

    // The projection (only needed if we are looping over directions)
    //const astro::ProjBase* m_proj;
    const astro::SkyProj* m_proj;

    // The direction of the test source (changes if we are looping over directions)
    mutable astro::SkyDir m_testSourceDir;  
 
    // Binners for the various loop parameters
    const evtbin::Binner* m_dir1_binner;
    const evtbin::Binner* m_dir2_binner;
    const evtbin::Binner* m_energy_binner;
    const evtbin::Binner* m_norm_binner;

    // The test source 
    Source* m_testSource;

    // The name of the test source (useful for bookkeeping)
    std::string m_testSourceName;

    // These are the output data from the scanning
    std::vector< std::pair< std::string,std::pair<HistND*,std::string> > > m_scanData;

    // This is where we cache all the info about the model and the fits
    FitScanCache* m_cache;

    // This is what we used to move around the image of the test source
    TestSourceModelCache* m_testSourceCache;    

  };

}

#endif // Likelihood_CountsMapHealpix_h
