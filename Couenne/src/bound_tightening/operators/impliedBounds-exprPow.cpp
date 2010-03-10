/* $Id$
 *
 * Name:    impliedBounds-exprPow.cpp
 * Author:  Pietro Belotti
 * Purpose: implied bounds for power operators
 *
 * (C) Carnegie-Mellon University, 2008-10.
 * This file is licensed under the Common Public License (CPL)
 */

#include "CouenneExprPow.hpp"
#include "CouenneExpression.hpp"
#include "CoinHelperFunctions.hpp"

/// set implied bounds for function w = x^k, k negative, integer or
/// inverse integer, and odd

void invPowImplBounds (int, int, CouNumber *, CouNumber *, CouNumber, bool &, bool &, enum expression::auxSign);


/// implied bound processing for expression w = x^k, upon change in
/// lower- and/or upper bound of w, whose index is wind

bool exprPow::impliedBound (int wind, CouNumber *l, CouNumber *u, t_chg_bounds *chg, enum expression::auxSign sign) {

  //int xi = arglist_ [0] -> Index ();
  //if (xi>=0) printf ("in implBound-pow: %g,%g\n", l [xi], u [xi]);

  //return false; // !!!

  bool resL, resU = resL = false;

  if (arglist_ [0] -> Type () == CONST)   // base is constant or zero, nothing to do
    return false;

  assert (arglist_ [1] -> Type () == CONST);

  int index = arglist_ [0] -> Index ();

  CouNumber k = arglist_ [1] -> Value (); // exponent

  if ((fabs (k) < COUENNE_EPS) || 
      (fabs (k) > COUENNE_INFINITY)) // a null or infinite k is of little use
    return false;

  int intk; // integer (or integer inverse of) exponent

  bool 
    isint    =           (fabs (k    - (intk = COUENNE_round (k)))    < COUENNE_EPS), // k   integer
    isinvint = !isint && (fabs (1./k - (intk = COUENNE_round (1./k))) < COUENNE_EPS); // 1/k integer

  CouNumber wl = sign == expression::GEQ ? -COIN_DBL_MAX : l [wind], // lower w
            wu = sign == expression::LEQ ?  COIN_DBL_MAX : u [wind]; // upper w

  if ((isint || isinvint) && (intk % 2)) { 

    // k or 1/k integer and odd, or non-integer --> it is a monotone
    // increasing function

    if (k > 0.) { // simple, just follow bounds

      if (wl > - COUENNE_INFINITY) resL = updateBound (-1, l + index, safe_pow (wl, 1./k)); 
      if (wu <   COUENNE_INFINITY) resU = updateBound (+1, u + index, safe_pow (wu, 1./k));

    } else // slightly more complicated, resort to same method as in exprInv
      invPowImplBounds (wind, index, l, u, 1./k, resL, resU, sign);
  } 
  else 
    if (isint) { // x^k, k integer and even --> non monotone

      CouNumber bound = (k<0) ? wl : wu;

      // |x| <= b^(1/k), where b is wl or wu depending on k negative
      // or positive, respectively

      if (bound > COUENNE_EPS) {

	if (fabs (bound) < COUENNE_INFINITY) {
	  resL = updateBound (-1, l + index, - safe_pow (bound, 1./k));
	  resU = updateBound (+1, u + index,   safe_pow (bound, 1./k));
	} /*else {
	  resL = updateBound (-1, l + index, - COUENNE_INFINITY);
	  resU = updateBound (+1, u + index,   COUENNE_INFINITY);
	  }*/
      }

      // invert check, if bounds on x do not contain 0 we may improve them

      bound = (k>0) ? wl : wu;

      CouNumber xl = l [index], 
	        xu = u [index],
                xb = safe_pow (bound, 1./k);

      if      (xl > - xb + COUENNE_EPS) resL = updateBound (-1, l + index,   xb) || resL;
      else if (xu <   xb - COUENNE_EPS) resU = updateBound ( 1, u + index, - xb) || resU;

    } else { 

      // Two cases:
      // 1) x^k, k=(1/h), h integer and even, or 
      // 2) x^k, neither k nor 1/k integer

      CouNumber lb = wl, ub = wu;

      if (k < 0) { // swap bounds as they swap on the curve x^k when 
	lb = wu;
	ub = wl;
      }

      if (lb > 0. || k > 0.) resL = updateBound (-1, l + index, safe_pow (lb, 1./k));

      if ((fabs (ub) < COUENNE_INFINITY) && (ub > 0 || k > 0.)) 
	resU = updateBound (+1, u + index, safe_pow (ub, 1./k));
      //else                  resU = updateBound (+1, u + index, COUENNE_INFINITY);
    }

  if (resL) chg [index].setLower(t_chg_bounds::CHANGED);
  if (resU) chg [index].setUpper(t_chg_bounds::CHANGED);

  bool xInt = arglist_ [0] -> isInteger ();

  if ((resL || resU) && xInt) {
    int xi = arglist_ [0] -> Index ();
    assert (xi >= 0);
    // careful with what "integer" means when a bound is 1e-8 (see minlp/deb[789].nl)
    if (resL && (fabs (l [xi]) > COUENNE_EPS)) l [xi] = ceil  (l [xi] - COUENNE_EPS);
    if (resU && (fabs (u [xi]) > COUENNE_EPS)) u [xi] = floor (u [xi] + COUENNE_EPS);
  }

  return (resL || resU);
}
