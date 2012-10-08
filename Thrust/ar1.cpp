#include "global.h"
#include <math.h>
#include <thrust/device_vector.h>

//////////////////////////////////////////////////////////////////////////////
///
/// @brief Function to compute discrete AR1 approximation values and
/// transition matrix.
///
/// @details This function that computes a discrete AR1 approximation and
/// transition matrix using the method of Tauchen (1986).
///
/// @param lambda upper and lower bounds on the AR1 grid in terms of number
/// of standard deviations from the mean.
/// @param Z pointer to array of AR1 grid values.
/// @param P pointer to array of AR1 transition matrix values.
///
/// @returns Void.
///
/// @author Eric M. Aldrich \n
///         ealdrich@ucsc.edu
///
/// @version 1.0
///
/// @date 24 July 2012
///
/// @copyright Copyright Eric M. Aldrich 2012 \n
///            Distributed under the Boost Software License, Version 1.0
///            (See accompanying file LICENSE_1_0.txt or copy at \n
///            http://www.boost.org/LICENSE_1_0.txt)
///
//////////////////////////////////////////////////////////////////////////////
void ar1(const parameters& param, thrust::device_vector<REAL>& Z,
	 thrust::device_vector<REAL>& P)
 {
  int ix,jx;

  // basic parameters
  const int nz = param.nz;
  const REAL mu = param.mu;
  const REAL rho = param.rho;
  const REAL sigma = param.sigma;
  const REAL lambda = param.lambda;

  // grid for TFP
  const REAL sigma_z = sigma/pow(1-pow(rho,2),0.5);
  const REAL mu_z = mu/(1-rho);
  const REAL zmin = mu_z - lambda*sigma_z;
  const REAL zmax = mu_z + lambda*sigma_z;
  const REAL zstep = (zmax - zmin)/(nz-1);
  for(ix = 0 ; ix < nz ; ++ix) Z[ix] = exp(zmin + zstep*ix);

  // transition matrix
  REAL normarg1, normarg2;
  for(ix = 0 ; ix < nz ; ++ix){
    normarg1 = (zmin - mu - rho*log(Z[ix]))/sigma + 0.5*zstep/sigma;
    P[ix*nz] = 0.5 + 0.5*erf(normarg1/sqrt((REAL)2));
    P[ix+nz*(nz-1)] = 1 - P[ix*nz];
    for(jx = 1 ; jx < (nz-1) ; ++jx){
      normarg1 = (log(Z[jx]) - mu - rho*log(Z[ix]))/sigma + 0.5*zstep/sigma;
      normarg2 = (log(Z[jx]) - mu - rho*log(Z[ix]))/sigma - 0.5*zstep/sigma;
      P[ix+nz*jx] = 0.5*erf(normarg1/sqrt((REAL)2)) - 0.5*erf(normarg2/sqrt((REAL)2));
      P[ix+nz*(nz-1)] -= P[ix+nz*jx];
    }
  }
}
