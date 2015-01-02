#include "cado.h"
#include "modredc_15ul_default.h"
#include <gmp.h>
#include "mpqs.h"
#include "mpqs_doit.h"

int
mpqs_15ul (modint_t f, const modulus_t m)
{
  mpz_t fz, mz;

  mpz_init (fz);
  mpz_init_set_ui (mz, m[0].m[1]);
  mpz_mul_2exp (mz, mz, LONG_BIT);
  mpz_add_ui (mz, mz, m[0].m[0]);
  mpqs_doit (fz, mz, 0);
  ASSERT_ALWAYS (mpz_fits_ulong_p (fz));
  f[0] = mpz_get_ui (fz);
  mpz_clear (fz);
  mpz_clear (mz);

  return 0; /* no back-tracking */
}