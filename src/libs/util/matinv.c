#include <stdlib.h>
#include <math.h>

/* ======================================================================= */
int matinv(double *array, int order, int dim)
{
  double amax, save, d1;
  int    i, j, k, *ik, *jk;

  if ((ik = malloc(order*sizeof(int))) == NULL ||
      (jk = malloc(order*sizeof(int))) == NULL) {
    free(ik);
    return -1;
  }
  for (k = 0; k < order; ++k) {
    /* find largest element array(i,j) in rest of matrix */
    amax = 0.0;
    while (1) {
      for (i = k; i < order; ++i) {
	for (j = k; j < order; ++j) {
	  if (fabs(amax) - (d1 = array[j*dim + i], fabs(d1)) <= 0.0) {
	    amax = array[j*dim + i];
	    ik[k] = i;
	    jk[k] = j;
	  }
	}
      }
      if (amax == 0.0) {
	free(ik);
	free(jk);
	return 0;
      }
      /* interchange rows and columns to put amax in array(k,k) */
      i = ik[k];
      if (i < k) continue;
      if (i > k) {
	for (j = 0; j < order; ++j) {
	  save = array[j*dim + k];
	  array[j*dim + k] = array[j*dim + i];
	  array[j*dim + i] = -save;
	}
      }
      j = jk[k];
      if (j >= k) break;
    }
    if (j > k) {
      for (i = 0; i < order; ++i) {
	save = array[k*dim + i];
	array[k*dim + i] = array[j*dim + i];
	array[j*dim + i] = -save;
      }
    }
    /* accumulate elements of inverse matrix */
    for (i = 0; i < order; ++i) {
      if (i != k) array[k*dim + i] = -array[k*dim + i] / amax;
    }
    for (i = 0; i < order; ++i) {
      for (j = 0; j < order; ++j) {
	if (i != k && j != k) array[j*dim + i] += array[k*dim + i] * array[j*dim + k];
      }
    }
    for (j = 0; j < order; ++j) {
      if (j != k) array[j*dim + k] /= amax;
    }
    array[k*dim + k] = 1.0 / amax;
  }
  /* restore ordering of matrix */
  for (k = order-1; k >= 0; --k) {
    j = ik[k];
    if (j > k) {
      for (i = 0; i < order; ++i) {
	save = array[k*dim + i];
	array[k*dim + i] = -array[j*dim + i];
	array[j*dim + i] = save;
      }
    }
    i = jk[k];
    if (i > k) {
      for (j = 0; j < order; ++j) {
	save = array[j*dim + k];
	array[j*dim + k] = -array[j*dim + i];
	array[j*dim + i] = save;
      }
    }
  }
  free(ik);
  free(jk);
  return 0;
} /* matinv */

/* ======================================================================= */
int matinv_float(float *array, int order, int dim)
{
  float amax, save, d1;
  int   i, j, k, *ik, *jk;

  if ((ik = malloc(order*sizeof(int))) == NULL ||
      (jk = malloc(order*sizeof(int))) == NULL) {
    free(ik);
    return -1;
  }
  for (k = 0; k < order; ++k) {
    /* find largest element array(i,j) in rest of matrix */
    amax = 0.0f;
    while (1) {
      for (i = k; i < order; ++i) {
	for (j = k; j < order; ++j) {
	  if (fabs(amax) - (d1 = array[j*dim + i], fabs(d1)) <= 0.0f) {
	    amax = array[j*dim + i];
	    ik[k] = i;
	    jk[k] = j;
	  }
	}
      }
      if (amax == 0.0f) {
	free(ik);
	free(jk);
	return 0;
      }
      /* interchange rows and columns to put amax in array(k,k) */
      i = ik[k];
      if (i < k) continue;
      if (i > k) {
	for (j = 0; j < order; ++j) {
	  save = array[j*dim + k];
	  array[j*dim + k] = array[j*dim + i];
	  array[j*dim + i] = -save;
	}
      }
      j = jk[k];
      if (j >= k) break;
    }
    if (j > k) {
      for (i = 0; i < order; ++i) {
	save = array[k*dim + i];
	array[k*dim + i] = array[j*dim + i];
	array[j*dim + i] = -save;
      }
    }
    /* accumulate elements of inverse matrix */
    for (i = 0; i < order; ++i) {
      if (i != k) array[k*dim + i] = -array[k*dim + i] / amax;
    }
    for (i = 0; i < order; ++i) {
      for (j = 0; j < order; ++j) {
	if (i != k && j != k) array[j*dim + i] += array[k*dim + i] * array[j*dim + k];
      }
    }
    for (j = 0; j < order; ++j) {
      if (j != k) array[j*dim + k] /= amax;
    }
    array[k*dim + k] = 1.0f / amax;
  }
  /* restore ordering of matrix */
  for (k = order-1; k >= 0; --k) {
    j = ik[k];
    if (j > k) {
      for (i = 0; i < order; ++i) {
	save = array[k*dim + i];
	array[k*dim + i] = -array[j*dim + i];
	array[j*dim + i] = save;
      }
    }
    i = jk[k];
    if (i > k) {
      for (j = 0; j < order; ++j) {
	save = array[j*dim + k];
	array[j*dim + k] = -array[j*dim + i];
	array[j*dim + i] = save;
      }
    }
  }
  free(ik);
  free(jk);
  return 0;
} /* matinv_float */
