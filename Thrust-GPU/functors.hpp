//////////////////////////////////////////////////////////////////////////////
///
/// @file functors.hpp
///
/// @brief File of Thrust functors and functions.
///
/// @author Eric M. Aldrich \n
///         ealdrich@ucsc.edu
///
/// @version 1.0
///
/// @date 12 July 2012
///
/// @copyright Copyright Eric M. Aldrich 2012 \n
///            Distributed under the Boost Software License, Version 1.0
///            (See accompanying file LICENSE_1_0.txt or copy at \n
///            http://www.boost.org/LICENSE_1_0.txt)
///
//////////////////////////////////////////////////////////////////////////////

#ifndef _FUNCTORS_H_
#define _FUNCTORS_H_

#include <thrust/iterator/zip_iterator.h>
#include <thrust/for_each.h>
#include <thrust/device_vector.h>
#include <cmath>

using namespace std;

//////////////////////////////////////////////////////////////////////////////
///
/// @brief Functor to compute discrete AR1 approximation.
///
/// @details This functor provides the kernel to compute a discrete AR1
/// approximation using the method of Tauchen (1986).
///
//////////////////////////////////////////////////////////////////////////////
template <typename T>
struct ar1Vals
{
  // Arguments
  const int nz; ///< Number of values in AR1 grid.
  const T lambda; ///< Number of standard deviations for AR1 approximation.
  const T mu; ///< AR1 mean.
  const T sigma; ///< AR1 standard deviation.
  const T rho; ///< AR1 persistence.

  /// Constructor
  ar1Vals(int _nz, T _lambda, T _mu, T _sigma, T _rho)
  : nz(_nz), lambda(_lambda), mu(_mu), sigma(_sigma), rho(_rho) {}

  /// Kernel to compute discrete AR1 approximation (using Tauchen's method).
  /// @param ix index of the AR1 grid.
  /// @return Value of the AR1 process at position ix in the grid.
  __host__ __device__
  T operator()(const int& ix) const 
  { 
    const T sigma_z = sigma/sqrt(1-pow(rho,2));
    const T mu_z = mu*(1/(1-rho));
    const T zmin = mu_z - lambda*sigma_z;
    const T zmax = mu_z + lambda*sigma_z;
    const T zstep = (zmax-zmin)/(nz-1);
    return (T)std::exp(zmin + zstep*ix);
  }
};

//////////////////////////////////////////////////////////////////////////////
///
/// @brief Functor to compute transition matrix for discrete AR1
/// approximation.
///
/// @details This functor provides the kernel to compute the transition matrix
/// for a a discrete AR1 approximation using the method of Tauchen (1986).
///
//////////////////////////////////////////////////////////////////////////////
template <typename T>
struct transMat
{
  // Attributes
  const int nz; ///< Number of values in AR1 grid.
  const T mu; ///< AR1 mean.
  const T sigma; ///< AR1 standard deviation.
  const T rho; ///< AR1 persistence.
  T* Z; ///< Pointer to AR1 grid.
  T* P; ///< Pointer to transition matrix.

  /// Constructor
  transMat(int _nz, T _mu, T _sigma, T _rho, T* _Z, T* _P)
  : nz(_nz), mu(_mu), sigma(_sigma), rho(_rho), Z(_Z), P(_P) {}

  /// Kernel to compute transition matrix for discrete AR1 approximation
  /// (using Tauchen's method).
  /// @param ix index of the AR1 grid.
  /// @return Void.
  __host__ __device__
  void operator()(const int& ix) const 
  { 
    const T zstep = (log(Z[nz-1])-log(Z[0]))/(nz-1);
    int jx;
    REAL normarg1, normarg2;
    normarg1 = (log(Z[0]) - mu - rho*log(Z[ix]))/sigma + 0.5*zstep/sigma;
    P[ix] = 0.5 + 0.5*erf(normarg1/pow(2,0.5));
    P[ix+nz*(nz-1)] = 1-P[ix];
    for(jx = 1 ; jx < nz-1 ; ++jx){
      normarg1 = (log(Z[jx]) - mu - rho*log(Z[ix]))/sigma + 0.5*zstep/sigma;
      normarg2 = (log(Z[jx]) - mu - rho*log(Z[ix]))/sigma - 0.5*zstep/sigma;
      P[ix+jx*nz] = 0.5*erf(normarg1/pow(2,0.5)) - 0.5*erf(normarg2/pow(2,0.5));
      P[ix+nz*(nz-1)] -= P[ix+jx*nz];
    }
  }

};

//////////////////////////////////////////////////////////////////////////////
///
/// @brief Functor to compute grid values for capital.
///
/// @details This functor provides the kernel to compute an equally spaced
/// grid for capital.
///
//////////////////////////////////////////////////////////////////////////////
template <typename T>
struct kGrid
{
  // Attributes
  const int nk; ///< Number of values in capital grid.
  const int nz; ///< Number of values in AR1 (TFP) grid.
  const T beta; ///< Time discount factor.
  const T alpha; ///< Capital share in production function.
  const T delta; ///< Depreciation rate.
  const T* Z; ///< Pointer to AR1 (TFP) grid.

  /// Constructor
  kGrid(int _nk, int _nz, T _beta, T _alpha, T _delta, T* _Z)
  : nk(_nk), nz(_nz), beta(_beta), alpha(_alpha), delta(_delta), Z(_Z) {}

  /// Kernel to compute each K value in grid.
  /// @param ix index of the AR1 grid.
  /// @return Value of capital at position ix in the grid.
  __host__ __device__
  T operator()(const int& ix) const 
  { 
    const T kmin = 0.95*pow((1/(alpha*Z[0]))*((1/beta)-1+delta),1/(alpha-1));
    const T kmax = 1.05*pow((1/(alpha*Z[nz-1]))*((1/beta)-1+delta),1/(alpha-1));
    const T kstep = (kmax-kmin)/(nk-1);
    return kmin + kstep*ix;
  }
};

//////////////////////////////////////////////////////////////////////////////
///
/// @brief Functor to initialize the value function.
///
/// @details This functor intializes the value function at the deterministic
/// steady state, for each value of the AR1 (TFP) grid.
///
//////////////////////////////////////////////////////////////////////////////
template <typename T>
struct vfInit
{
  // Attributes
  const int nk; ///< Number of values in capital grid.
  const T eta; ///< Coefficient of relative risk aversion.
  const T beta; ///< Time discount factor.
  const T alpha; ///< Capital share in production function.
  const T delta; ///< Depreciation rate.
  const T* Z; ///< Pointer to AR1 (TFP) grid.
  T* V; ///< Pointer to current iteration of the value function.

  /// Constructor
  vfInit(int _nk, T _eta, T _beta, T _alpha, T _delta, T* _Z, T* _V)
  : nk(_nk), eta(_eta), beta(_beta), alpha(_alpha), delta(_delta), Z(_Z), V(_V) {}

  /// Kernel to initialize value function.
  /// @param jx index of the AR1 (TFP) grid.
  /// @return Void.
  __host__ __device__
  void operator()(const int& jx) const 
  {
    int ix;
    const T Kj = pow((1/(alpha*Z[jx]))*((1/beta)-1+delta),1/(alpha-1));
    const T Vj = pow(Z[jx]*pow(Kj, alpha) - delta*Kj,1-eta)/(1-eta);
    for(ix = 0 ; ix < nk ; ++ix) V[ix+jx*nk] = Vj;
  }
};

//////////////////////////////////////////////////////////////////////////////
///
/// @brief Functor to update the value function.
///
/// @details This functor updates the value function. If @link howard @endlink
/// is FALSE, the value and capital policy functions are updated by
/// maximizing the Belman objective function using the current value function.
/// Maximization is either performed by @link binary_max @endlink or
/// @link grid_max @endlink. If @link howard @endlink is TRUE, the value
/// function is updated without maximization, by simply iterating the Belman
/// with the current capital policy function.
///
//////////////////////////////////////////////////////////////////////////////
template <typename T>
struct vfStep
{
  // Attributes
  const int nk; ///< Number of values in capital grid.
  const int nz; ///< Number of values in AR1 (TFP) grid.
  const T eta; ///< Coefficient of relative risk aversion.
  const T beta; ///< Time discount factor.
  const T alpha; ///< Capital share in production function.
  const T delta; ///< Depreciation rate.
  const char maxtype; ///< Flag to indicate maximization method.
  const bool howard; ///< Flag to indicate use of Howard improvement.
  const T* K; ///< Pointer to capital grid.
  const T* Z; ///< Pointer to AR1 (TFP) grid.
  const T* P; ///< Pointer to transition matrix.
  const T* V0; ///< Pointer to current iteration of the value function.
  T* V; ///< Pointer to the updated value function.
  T* G; ///< Pointer to current iteration of the capital policy function.

  /// Constructor
  vfStep(int _nk, int _nz, T _eta, T _beta, T _alpha, T _delta, char _maxtype,
	 bool _howard, T* _K, T* _Z, T* _P, T* _V0, T* _V, T* _G)
    : nk(_nk), nz(_nz), eta(_eta), beta(_beta), alpha(_alpha), delta(_delta),
      maxtype(_maxtype), howard(_howard), K(_K), Z(_Z), P(_P), V0(_V0), V(_V),
      G(_G) {}

  /// Kernel to update the value function.
  /// @param hx index of V0 (stored as a flat array).
  /// @return Void.
  __host__ __device__
  void operator()(const int& hx) const 
  {

    // Compute the row and column IDs
    int ix = hx%nk;
    int jx = (hx-ix)/nk;

    // Output and depreciated capital
    const T ydepK = Z[jx]*pow(K[ix],alpha) + (1-delta)*K[ix];

    // maximize on non-howard steps
    if(howard == false){
      
      // impose constraints on grid for future capital
      const int klo = 0;
      int khi = binary_val(ydepK, nk, &K[0]); // nonnegativity of C
      if(K[khi] > ydepK) khi -= 1;
      const int nksub = khi-klo+1;

      // maximization either via grid (g), or binary search (b)
      // if binary, turn off policy iteration (to preserve concavity)
      if(maxtype == 'g'){
	grid_max(klo, nksub, nk, nz, ydepK, eta, beta, &K[0], (&P[0]+jx),
		 (&V0[0]+klo), (&V[0]+ix+jx*nk), (&G[0]+ix+jx*nk));
      } else if(maxtype == 'b'){
	binary_max(klo, nksub, nk, nz, ydepK, eta, beta, &K[0], (&P[0]+jx),
	   (&V0[0]+klo), (&V[0]+ix+jx*nk), (&G[0]+ix+jx*nk));
      }

      // iterate on the policy function on non-howard steps
    } else {
      T Exp = 0.0;
      for(int mx = 0 ; mx < nz ; ++mx) Exp += P[jx+mx*nz]*V0[(int)G[ix+jx*nk]+mx*nk];
      V[ix+jx*nk] = pow(ydepK-K[(int)G[ix+jx*nk]],1-eta)/(1-eta) + beta*Exp;
    }
  }
};

//////////////////////////////////////////////////////////////////////////////
///
/// @brief Function to find the location of a value in a monotonic grid.
///
/// @details This function finds the first value X[ix] such that X[ix] >= x,
/// where x is a scalar value, X is a monotonic grid, and ix is the index
/// of X.
///
/// @param x value to search for in grid X.
/// @param n number of values in grid X.
/// @param *X pointer to grid X.
/// @return imax first integer (<= n) such that X[ix] >= x. 
///
//////////////////////////////////////////////////////////////////////////////
template <typename T>
__host__ __device__
int binary_val(const T x, const int n, const T* X)
{

  int imax;

  // check if x is out of bounds
  if(x < X[0]){
    imax = 0;
    return imax;
  }
  if(x > X[n-1]){
    imax = n-1;
    return imax;
  }

  // otherwise
  int ilo, ihi, imid;
  ilo = 0;
  ihi = n-1;
  while((ihi-ilo) > 1){
    imid = (ilo + ihi)/2;
    if(X[imid] == x){
      imax = imid;
      return imax;
    } else if(X[imid] > x){
      ihi = imid;
    } else ilo = imid;
  }  
  imax = ihi;
  return imax;
}

//////////////////////////////////////////////////////////////////////////////
///
/// @brief Function to maximize Belman objective function with naive grid
/// search.
///
/// @details This function computes the maximum of the Belman objective
/// function for a given pair of state values by searching over each
/// possible value of future capital in the grid for capital.
///
/// @param klo lower index of the capital grid to begin search.
/// @param nksub number of points in the capital grid to include in search.
/// @param nk number of points in the capital grid.
/// @param nz number of points in the AR1 (TFP) grid.
/// @param ydepK value of output plus depreciated capital.
/// @param eta coefficient of relative risk aversion.
/// @param beta time discount factor.
/// @param *K pointer to capital grid.
/// @param *P pointer to AR1 (TFP) transition matrix.
/// @param *V0 pointer to current iterate of value function.
/// @param *V pointer to updated value function.
/// @param *G pointer to updated capital policy function.
/// @return Void.
///
//////////////////////////////////////////////////////////////////////////////
template <typename T>
__host__ __device__
void grid_max(const int klo, const int nksub, const int nk,
	      const int nz, const T ydepK, const T eta,
	      const T beta, const T* K, const T* P,
	      const T* V0, T* V, T* G)
{
  T Exp = 0.0, w, wmax;
  int l, m, windmax;
  for(m = 0 ; m < nz ; ++m) Exp += (*(P+m*nz))*(*(V0+m*nk));
  w = pow(ydepK-K[klo],1-eta)/(1-eta) + beta*Exp;
  wmax = w;
  windmax = 0;
  for(l = 1 ; l < nksub ; ++l){
    Exp = 0.0;
    for(m = 0 ; m < nz ; ++m) Exp += (*(P+m*nz))*(*(V0+l+m*nk));
    w = pow(ydepK-K[klo+l],1-eta)/(1-eta) + beta*Exp;
    if(w > wmax){
      wmax = w;
      windmax = l;
    }
  }
  *V = wmax;
  *G = klo+windmax;
}

//////////////////////////////////////////////////////////////////////////////
///
/// @brief Function to maximize Belman objective function with binary search.
///
/// @details This function computes the maximum of the Belman objective
/// function for a given pair of state values by using a binary search
/// algorithm, as outlined in Heer and Maussner (2005, p.26).
///
/// @param klo lower index of the capital grid to begin search.
/// @param nksub number of points in the capital grid to include in search.
/// @param nk number of points in the capital grid.
/// @param nz number of points in the AR1 (TFP) grid.
/// @param ydepK value of output plus depreciated capital.
/// @param eta coefficient of relative risk aversion.
/// @param beta time discount factor.
/// @param *K pointer to capital grid.
/// @param *P pointer to AR1 (TFP) transition matrix.
/// @param *V0 pointer to current iterate of value function.
/// @param *V pointer to updated value function.
/// @param *G pointer to updated capital policy function.
/// @return Void.
///
//////////////////////////////////////////////////////////////////////////////
template <typename T>
__host__ __device__
void binary_max(const int klo, const int nksub, const int nk,
		const int nz, const T ydepK, const T eta,
		const T beta, const T* K, const T* P,
		const T* V0, T* V, T* G)
{
  // binary search to find the vf max over K'
  // we assume that the value funtion is concave in capital
  int kslo, kshi, ksmid1, ksmid2, l;
  T Exp1, Exp2, w1, w2;
  kslo = 0;
  kshi = nksub-1;
   
  // case 1: capital grid has more than three values
  if(nksub > 3){
    // while the grid has 3 values or more, compute vf at midpoints
    // and revise the bounds of the grid
    while(kshi-kslo > 2){
      ksmid1 = (kslo + kshi)/2;
      ksmid2 = ksmid1+1;
      Exp1 = 0.0;
      Exp2 = 0.0;
      for(l = 0 ; l < nz ; ++l){
  	Exp1 += (*(P+l*nz))*(*(V0+ksmid1+l*nk));
  	Exp2 += (*(P+l*nz))*(*(V0+ksmid2+l*nk));
      }
      w1 = pow(ydepK-K[klo+ksmid1],1-eta)/(1-eta) + beta*Exp1;
      w2 = pow(ydepK-K[klo+ksmid2],1-eta)/(1-eta) + beta*Exp2;
      if(w2 > w1) kslo = ksmid1; else kshi = ksmid2;
    }
    // when the grid is reduced to three values, find the max
    if(w2 > w1){
      Exp1 = 0.0;
      for(l = 0 ; l < nz ; ++l) Exp1 += (*(P+l*nz))*(*(V0+kshi+l*nk));
      w1 = pow(ydepK-K[klo+kshi],1-eta)/(1-eta) + beta*Exp1;
      if(w2 > w1){
  	*V = w2; *G = klo+kslo+1;
      } else {
  	*V = w1; *G = klo+kshi;
      }
    } else {
      Exp2 = 0.0;
      for(l = 0 ; l < nz ; ++l) Exp2 += (*(P+l*nz))*(*(V0+kslo+l*nk));
      w2 = pow(ydepK-K[klo+kslo],1-eta)/(1-eta) + beta*Exp2;
      if(w2 > w1){
  	*V = w2; *G = klo+kslo;
      } else {
  	*V = w1; *G = klo+kslo+1;
      }
    }
  	
  // case 2: capital grid has three values
  } else if(nksub == 3) {
    // evaluate vf at each value and determine max
    Exp1 = 0.0, Exp2 = 0.0;
    T Exp3 = 0.0;
    for(l = 0 ; l < nz ; ++l){
      Exp1 += (*(P+l*nz))*(*(V0+kslo+l*nk));
      Exp2 += (*(P+l*nz))*(*(V0+kslo+1+l*nk));
      Exp3 += (*(P+l*nz))*(*(V0+kshi+l*nk));
    }
    w1 = pow(ydepK-K[klo+kslo],1-eta)/(1-eta) + beta*Exp1;
    w2 = pow(ydepK-K[klo+kslo+1],1-eta)/(1-eta) + beta*Exp2;
    const T w3 = pow(ydepK-K[klo+kshi],1-eta)/(1-eta) + beta*Exp3;
    *V = w1;
    *G = klo+kslo;
    if(w2 > *V){*V = w2; *G = klo+kslo+1;}
    if(w3 > *V){ *V = w3; *G = klo+kshi;}
  	
  // case 3: capital grid has one or two values
  } else {
    Exp1 = 0.0, Exp2 = 0.0;
    for(l = 0 ; l < nz ; ++l){
      Exp1 += (*(P+l*nz))*(*(V0+kslo+l*nk));
      Exp2 += (*(P+l*nz))*(*(V0+kshi+l*nk));
    }
    // evaluate vf at each value and determine max
    w1 = pow(ydepK-K[klo+kslo],1-eta)/(1-eta) + beta*Exp1;
    w2 = pow(ydepK-K[klo+kshi],1-eta)/(1-eta) + beta*Exp2;
    if(w2 > w1){
      *V = w2; *G = klo+kshi;
    } else {
      *V = w1; *G = klo+kslo;
    }
  }
}

//////////////////////////////////////////////////////////////////////////////
///
/// @brief Functor to compute the absolute difference between elements of
/// two vectors.
///
//////////////////////////////////////////////////////////////////////////////
template <typename T>
struct abs_diff
{

  /// Kernel to compute the absolute difference between elements.
  /// @param x value of first vector element.
  /// @param y value of second vector element.
  /// @return absolute difference between elements.
  __host__ __device__
  T operator()(const T& x, const T& y) const { 
    return fabs(x - y);
  }
};

#endif