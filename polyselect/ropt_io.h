#ifndef ROPT_IO_H
#define ROPT_IO_H

#include "utils.h"
#include "murphyE.h"
#include "ropt_param.h"

/* -- declarations -- */

/* L1 cache detection */
int cachesize_cpuid ( int ); // from utils

int cachesize_guess ( int ); // from utils

void ropt_L1_cachesize ();


/* ropt on polys in formats cado or msieve or from stdin */
void ropt_on_stdin ( ropt_param_t param );

void ropt_on_cadopoly ( FILE *file,
                        ropt_param_t param );

void ropt_on_msievepoly ( FILE *file,
                          ropt_param_t param );


/* parse stage 2 parameters from argv */
void ropt_parse_param ( int argc,
                        char **argv,
                        ropt_param_t param );






#if SKIP_ROPT
double print_poly_info_short ( mpz_t *f,
                               mpz_t *g,
                               int d,
                               mpz_t N );
#endif

#endif
