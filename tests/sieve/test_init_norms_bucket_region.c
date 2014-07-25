#define NB_TESTS 20      /* NB_TESTS for a given degree & given logi */
#define MIN_LOGI 10
#define MAX_LOGI 16      /* MIN_LOGI <= logi <= MAX_LOGI && MAX_LOGI <= LOG_BUCKET_REGION */
#define MAX_DEGREE 10    /* All degree between 0 and MAX_DEGREE included are tested */

#define MAX_SMART_ERR 8            /* The error in smart algo must be <=, always. */
#define TOLERATE_MAX_SMART_ERR 4   /* The error in smart algo COULD be > ... */
#define MAX_RELATIVE_ERROR 1000000 /* ... but only for 1 value on MAX_RELATIVE_ERROR values. */

#define MAX_EXACT_ERR 2  /* The error in "exact" algo must be <= */

#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>

#include "cado_config.h"
#include "utils/utils.h"
#include "portability.h"
#include "tests/tests_common.h"
#include "sieve/las-config.h"
#include "sieve/las-types.h"
#include "sieve/las-debug.h"
#include "sieve/las-norms.h"

static inline void
my_poly_scale (double *u, const double *t, unsigned int d, double h)
{
  u[d] = t[d];
  for (double hpow = h; d--; hpow *= h) u[d] = t[d] * hpow;
}

int
main(int argc, const char *argv[]) {
  size_t l, logI, k, d;
  uint32_t J, beginJ, endJ;
  double scale, log2max, h, g;
  unsigned char *S1 = malloc_pagealigned((1U<<LOG_BUCKET_REGION) + MEMSET_MIN), 
    *S2 = malloc_pagealigned((1U<<LOG_BUCKET_REGION) + MEMSET_MIN),
    *S3 = malloc_pagealigned((1U<<LOG_BUCKET_REGION) + MEMSET_MIN), *cS3;
  double_poly_t poly;
  unsigned long long count, smart_err[(MAX_SMART_ERR<<1)+1], exact_err[(MAX_EXACT_ERR<<1)+1];
  unsigned long iter = NB_TESTS;
  unsigned long long smart_errors = 0;
  
  tests_common_cmdline (&argc, &argv, PARSE_SEED | PARSE_ITER | PARSE_VERBOSE);
  tests_common_get_iter (&iter);

  memset(smart_err, 0, sizeof(smart_err));
  memset(exact_err, 0, sizeof(exact_err));
  ASSERT_ALWAYS (MIN_LOGI <= MAX_LOGI);
  ASSERT_ALWAYS (MAX_LOGI <= LOG_BUCKET_REGION);
  ASSERT_ALWAYS (S1);
  ASSERT_ALWAYS (S2);
  for (logI = MIN_LOGI; logI <= MAX_LOGI; logI++) {
    const uint32_t I = 1U << logI;
    const double didiv2 = (double) (I >> 1);
    for (d = MAX_DEGREE + 1; d--;) {
      poly->deg = d;
      double coeff[poly->deg + 1] __attribute__((aligned(16))), u[poly->deg + 1];
      struct root_s roots[4 * poly->deg + 1];
      unsigned int nroots;
      poly->coeff = coeff;
      for (k = iter; k--;) {
	J = ((uint32_t) random_uint64()) << 2;    /* 0 <= J < 2^30 at least */
	J &= (I >> 1) - 1;                        /* Original J; 0 <= J < I/2 */
	for (l = poly->deg + 1; l--;) {           /* (-2^63)^2 <= coeff <= (2^63-1)^2 */
	  poly->coeff[l] = (double) random_int64 () * (double) random_int64 ();
	  if (UNLIKELY(poly->coeff[l] == 0.)) ++l; /* coef must be != 0. */
	}

	endJ = (LOG_BUCKET_REGION - logI);
	J >>= endJ;                 /* In order to be able to compute a complete region */
	log2max = log2(fabs(get_maxnorm_alg (poly, didiv2, didiv2) + 1.));
	scale = (UCHAR_MAX - 1 - GUARD) / log2max; /* In order to have [GUARD, UCHAR_MAX-1] */
	
	/* Phase 1: compute S1 & S2 */
	memset (S1, 0, 1U<<LOG_BUCKET_REGION); /* To be sure S1 will be computed */
	memset (S2, 0, 1U<<LOG_BUCKET_REGION); /* To be sure S2 will be computed */
	if (poly->deg > 1) {
	  init_norms_roots_internal (poly->deg, poly->coeff, (double) ((I + 16) >> 1), 1. / (double) (I >> 1), &nroots, roots);
	  init_smart_degree_X_norms_bucket_region_internal (S1, J, I, scale, poly->deg, poly->coeff, nroots, roots);
	  init_exact_degree_X_norms_bucket_region_internal (S2, J, I, scale, poly->deg, poly->coeff);
	} else if (poly->deg == 1) {
	  double cexp2[257], step, iter;
	  step = 1. / scale;
	  iter = -step * GUARD;
	  for (l = 0; l < 257; iter += step) cexp2[l++] = exp2 (iter);
	  init_degree_one_norms_bucket_region_internal (S1, J, I, scale, poly->coeff[0], poly->coeff[1], cexp2);
	  memcpy (S2, S1, 1U<<LOG_BUCKET_REGION);
	} else {
	  memset (S1, (int) (log2(1.+fabs(poly->coeff[0]))*scale)+GUARD, 1U<<LOG_BUCKET_REGION);
	  memcpy (S2, S1, 1U<<LOG_BUCKET_REGION);
	}
	
	/* Phase 2: compute S3 */
	memset (S3, 0, 1U<<LOG_BUCKET_REGION); /* To be sure S3 will be computed */
	J <<= endJ;
	endJ = (1U << endJ) + J;
	cS3 = S3;
	for (beginJ = J; beginJ < endJ; ++beginJ) {
	  my_poly_scale (u, poly->coeff, poly->deg, (double) beginJ);
	  for (h = -didiv2; h < didiv2; h += 1.) {
	    for (l = poly->deg, g = u[l]; l--; g = g * h + u[l]);
	    g = fabs(g) + 1.;
	    g = log2(g) * scale + GUARD; /* log2(|x|+1.) >= 0 */
	    ASSERT_ALWAYS (g < (double) UCHAR_MAX);
	    *cS3++ = g;
	  }
	}
	/* Phase 3: compare (S1, S3) && (S2, S3) */
	for (l = 0; l < (1U<<LOG_BUCKET_REGION); l++) {
	  int test_exact_algo, test_smart_algo;

	  test_exact_algo = S3[l] - S2[l];
	  if (UNLIKELY(abs(test_exact_algo) > MAX_EXACT_ERR)) {
	    fprintf (stderr, 
		     "code BUG(): the condition abs(test_exact_algo) <= MAX_EXACT_ERR failed!\n"
		     "test_exact_algo = %d, maximal error accepted in exact algorithm = %u\n",
		     test_exact_algo, MAX_EXACT_ERR);
	    exit (EXIT_FAILURE);
	  }
	  exact_err[test_exact_algo + MAX_EXACT_ERR]++;

	  test_smart_algo = S3[l] - S1[l];
	  if (UNLIKELY(abs(test_smart_algo) > TOLERATE_MAX_SMART_ERR)) {
	    if (UNLIKELY(abs(test_smart_algo) > MAX_SMART_ERR)) {
	      fprintf (stderr, 
		       "code BUG(): the condition abs(test_smart_algo) <= MAX_SMART_ERR failed!\n"
		       "test_smart_algo=%d, maximal error accepted in smart algorithm = %u\n",
		       test_smart_algo, MAX_SMART_ERR);
	      exit (EXIT_FAILURE);
	    }
	    smart_errors++;
	  }
	  smart_err[test_smart_algo + MAX_SMART_ERR]++;
	}
      }
    }
  }
  free(S1); free(S2); free (S3);

  count = (MAX_LOGI - MIN_LOGI + 1) * (MAX_DEGREE + 1) * (unsigned long long) (iter << LOG_BUCKET_REGION);
  if (smart_errors * MAX_RELATIVE_ERROR > count) {
    fprintf (stderr, 
	     "code BUG(): too many tolerate errors (%llu errors for\n"
	     "%llu values) in the smart initialization algorithm : %llu errors.\n"
	     "NB: %u < tolerate errors <= %u\n",
	     count / MAX_RELATIVE_ERROR, count, smart_errors, TOLERATE_MAX_SMART_ERR, MAX_SMART_ERR);
    exit (EXIT_FAILURE);
  }
  
  if (tests_common_get_verbose ()) {
    fprintf (stderr, "This test is done with %llu values. All the values are between %u and %u included.\n", count, GUARD, UCHAR_MAX - 1);
#ifdef HAVE_GCC_STYLE_AMD64_INLINE_ASM
    fprintf (stderr, "Computations done with SSE (HAVE_GCC_STYLE_AMD64_INLINE_ASM is defined).\n");
#else
    fprintf (stderr, "Computations done with doubles (HAVE_GCC_STYLE_AMD64_INLINE_ASM is not defined).\n");
#endif
    double lc = 100. / count;

    fprintf (stderr, "\nDifferences between the exact algorithm and\nthe exact algorithm + fast log2 :\nNo difference: %6.3f%%\n", exact_err[MAX_EXACT_ERR] * lc);
    for (ssize_t y = 1; y <= MAX_EXACT_ERR; y++)
      if (exact_err[MAX_EXACT_ERR + y] || exact_err[MAX_EXACT_ERR - y])
	fprintf (stderr, "Difference %2zu: Negative (bad): %6.3f%% (%10llu); Positive (acceptable): %6.3f%% (%10llu)\n", y, exact_err[MAX_EXACT_ERR - y] * lc, exact_err[MAX_EXACT_ERR - y], exact_err[MAX_EXACT_ERR + y] * lc, exact_err[MAX_EXACT_ERR + y]);
    
    fprintf (stderr, "\nDifferences between the exact algorithm and\nthe smart algorithm + roots neighborhood correction + fast log2 :\nNo difference: %6.3f%%\n", smart_err[MAX_SMART_ERR] * lc);
    for (ssize_t y = 1; y <= MAX_SMART_ERR; y++)
      if (smart_err[MAX_SMART_ERR + y] || smart_err[MAX_SMART_ERR - y])
	fprintf (stderr, "Difference %2zu: Negative (bad): %6.3f%% (%10llu); Positive (acceptable): %6.3f%% (%10llu)\n", y, smart_err[MAX_SMART_ERR - y] * lc, smart_err[MAX_SMART_ERR - y], smart_err[MAX_SMART_ERR + y] * lc, smart_err[MAX_SMART_ERR + y]);
  }

  tests_common_clear();
  exit (EXIT_SUCCESS);
}
