/**
 *
 * \section COPYRIGHT
 *
 * Copyright 2013-2014 The libLTE Developers. See the
 * COPYRIGHT file at the top-level directory of this distribution.
 *
 * \section LICENSE
 *
 * This file is part of the libLTE library.
 *
 * libLTE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * libLTE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */


#include <float.h>
#include <complex.h>
#include <stdlib.h>
#include <string.h>

#include "liblte/phy/utils/vector.h"
#include "liblte/phy/utils/bit.h"

#ifdef HAVE_VOLK
#include "volk/volk.h"
#endif

int vec_acc_ii(int *x, uint32_t len) {
  int i;
  int z=0;
  for (i=0;i<len;i++) {
    z+=x[i];
  }
  return z;
}

float vec_acc_ff(float *x, uint32_t len) {
#ifdef HAVE_VOLK_ACC_FUNCTION
  float result;
  volk_32f_accumulator_s32f(&result,x,len);
  return result;
#else
  int i;
  float z=0;
  for (i=0;i<len;i++) {
    z+=x[i];
  }
  return z;
#endif
}

cf_t vec_acc_cc(cf_t *x, uint32_t len) {
  int i;
  cf_t z=0;
  for (i=0;i<len;i++) {
    z+=x[i];
  }
  return z;
}

void vec_sub_fff(float *x, float *y, float *z, uint32_t len) {
#ifndef HAVE_VOLK_SUB_FLOAT_FUNCTION
  int i;
  for (i=0;i<len;i++) {
    z[i] = x[i]-y[i];
  }
#else
  volk_32f_x2_subtract_32f(z,x,y,len);
#endif 
}

void vec_sum_ccc(cf_t *x, cf_t *y, cf_t *z, uint32_t len) {
  int i;
  for (i=0;i<len;i++) {
    z[i] = x[i]+y[i];
  }
}

void vec_sum_bbb(char *x, char *y, char *z, uint32_t len) {
  int i;
  for (i=0;i<len;i++) {
    z[i] = x[i]+y[i];
  }
}

void vec_sc_prod_fff(float *x, float h, float *z, uint32_t len) {
#ifndef HAVE_VOLK_MULT_FLOAT_FUNCTION
  int i;
  for (i=0;i<len;i++) {
    z[i] = x[i]*h;
  }
#else
  volk_32f_s32f_multiply_32f(z,x,h,len);
#endif
}

void vec_sc_prod_cfc(cf_t *x, float h, cf_t *z, uint32_t len) {
#ifndef HAVE_VOLK_MULT_FUNCTION
  int i;
  for (i=0;i<len;i++) {
    z[i] = x[i]*h;
  }
#else
  cf_t hh;
  __real__ hh = h;
  __imag__ hh = 0;
  volk_32fc_s32fc_multiply_32fc(z,x,hh,len);
#endif
}

void vec_sc_prod_ccc(cf_t *x, cf_t h, cf_t *z, uint32_t len) {
#ifndef HAVE_VOLK_MULT_FUNCTION
  int i;
  for (i=0;i<len;i++) {
    z[i] = x[i]*h;
  }
#else
  volk_32fc_s32fc_multiply_32fc(z,x,h,len);
#endif
}

void vec_convert_fi(float *x, int16_t *z, float scale, uint32_t len) {
#ifdef HAVE_VOLK_CONVERT_FI_FUNCTION
  volk_32f_s32f_convert_16i(z, x, scale, len);
#else 
  int i;
  for (i=0;i<len;i++) {
    z[i] = (int16_t) (x[i]*scale);
  }
#endif
}

void vec_deinterleave_cf(cf_t *x, float *real, float *imag, uint32_t len) {
 #ifdef HAVE_VOLK_DEINTERLEAVE_FUNCTION
  volk_32fc_deinterleave_32f_x2(real, imag, x, len);
#else 
  int i;
  for (i=0;i<len;i++) {
    real[i] = __real__ x[i];
    imag[i] = __imag__ x[i];
  }
#endif 
}

void *vec_malloc(uint32_t size) {
#ifndef HAVE_VOLK
  return malloc(size);
#else
  void *ptr;
  if (posix_memalign(&ptr,volk_get_alignment(),size)) {
    return NULL;
  } else {
    return ptr;
  }
#endif
}

void *vec_realloc(void *ptr, uint32_t old_size, uint32_t new_size) {
#ifndef HAVE_VOLK
  return realloc(ptr, new_size);
#else
  void *new_ptr;
  if (posix_memalign(&new_ptr,volk_get_alignment(),new_size)) {
    return NULL;
  } else {
    memcpy(new_ptr, ptr, old_size);
    free(ptr);
    return new_ptr;
  }
#endif
}


void vec_fprint_c(FILE *stream, cf_t *x, uint32_t len) {
  int i;
  fprintf(stream, "[");
  for (i=0;i<len;i++) {
    fprintf(stream, "%+2.2f%+2.2fi, ", __real__ x[i], __imag__ x[i]);
  }
  fprintf(stream, "];\n");
}

void vec_fprint_f(FILE *stream, float *x, uint32_t len) {
  int i;
  fprintf(stream, "[");
  for (i=0;i<len;i++) {
    fprintf(stream, "%+2.2f, ", x[i]);
  }
  fprintf(stream, "];\n");
}


void vec_fprint_b(FILE *stream, char *x, uint32_t len) {
  int i;
  fprintf(stream, "[");
  for (i=0;i<len;i++) {
    fprintf(stream, "%d, ", x[i]);
  }
  fprintf(stream, "];\n");
}

void vec_fprint_i(FILE *stream, int *x, uint32_t len) {
  int i;
  fprintf(stream, "[");
  for (i=0;i<len;i++) {
    fprintf(stream, "%d, ", x[i]);
  }
  fprintf(stream, "];\n");
}

void vec_fprint_hex(FILE *stream, char *x, uint32_t len) {
  uint32_t i, nbytes, byte;
  nbytes = len/8;
  fprintf(stream, "[", len);
  for (i=0;i<nbytes;i++) {
    byte = bit_unpack(&x, 8);
    fprintf(stream, "%02x ", byte);
  }
  if (len%8) {
    byte = bit_unpack(&x, len%8);
    fprintf(stream, "%02x ", byte);
  }
  fprintf(stream, "];\n");
}

void vec_save_file(char *filename, void *buffer, uint32_t len) {
  FILE *f; 
  f = fopen(filename, "w");
  if (f) {
    fwrite(buffer, len, 1, f);
    fclose(f);
  } else {
    perror("fopen");
  }  
}

void vec_conj_cc(cf_t *x, cf_t *y, uint32_t len) {
#ifndef HAVE_VOLK_CONJ_FUNCTION
  int i;
  for (i=0;i<len;i++) {
    y[i] = conjf(x[i]);
  }
#else
  volk_32fc_conjugate_32fc(y,x,len);
#endif
}

void vec_prod_cfc(cf_t *x, float *y, cf_t *z, uint32_t len) {
#ifndef HAVE_VOLK_MULT_REAL_FUNCTION
  int i;
  for (i=0;i<len;i++) {
    z[i] = x[i]*y[i];
  }
#else
  volk_32fc_32f_multiply_32fc(z,x,y,len);
#endif
}


void vec_prod_ccc(cf_t *x,cf_t *y, cf_t *z, uint32_t len) {
#ifndef HAVE_VOLK_MULT2_FUNCTION
  int i;
  for (i=0;i<len;i++) {
    z[i] = x[i]*y[i];
  }
#else
  volk_32fc_x2_multiply_32fc(z,x,y,len);
#endif
}


void vec_prod_conj_ccc(cf_t *x,cf_t *y, cf_t *z, uint32_t len) {
#ifndef HAVE_VOLK_MULT2_CONJ_FUNCTION
  int i;
  for (i=0;i<len;i++) {
    z[i] = x[i]*conjf(y[i]);
  }
#else
  volk_32fc_x2_multiply_conjugate_32fc(z,x,y,len);
#endif
}

void vec_div_ccc(cf_t *x, cf_t *y, cf_t *z, uint32_t len) {
  int i;
  for (i=0;i<len;i++) {
    z[i] = x[i] / y[i];
  }
}

void vec_div_fff(float *x, float *y, float *z, uint32_t len) {
#ifdef HAVE_VOLK_DIVIDE_FUNCTION
  volk_32f_x2_divide_32f(z, x, y, len);
#else
  int i;
  for (i=0;i<len;i++) {
    z[i] = x[i] / y[i];
  }
#endif
}

cf_t vec_dot_prod_ccc(cf_t *x, cf_t *y, uint32_t len) {
#ifdef HAVE_VOLK_DOTPROD_FC_FUNCTION
  cf_t res;
  volk_32fc_x2_dot_prod_32fc(&res, x, y, len);
  return res; 
#else 
  uint32_t i;
  cf_t res = 0;
  for (i=0;i<len;i++) {
    res += x[i]*y[i];
  }
  return res;
#endif
}

cf_t vec_dot_prod_conj_ccc(cf_t *x, cf_t *y, uint32_t len) {
#ifdef HAVE_VOLK_DOTPROD_CONJ_FC_FUNCTION
  cf_t res;
  volk_32fc_x2_conjugate_dot_prod_32fc(&res, x, y, len);
  return res; 
#else 
  uint32_t i;
  cf_t res = 0;
  for (i=0;i<len;i++) {
    res += x[i]*conjf(y[i]);
  }
  return res;
#endif
}


float vec_dot_prod_fff(float *x, float *y, uint32_t len) {
#ifdef HAVE_VOLK_DOTPROD_F_FUNCTION
  float res;
  volk_32f_x2_dot_prod_32f(&res, x, y, len);
  return res; 
#else 
  uint32_t i;
  float res = 0;
  for (i=0;i<len;i++) {
    res += x[i]*y[i];
  }
  return res;
#endif  
}


float vec_avg_power_cf(cf_t *x, uint32_t len) {
  int j;
  float power = 0;
  for (j=0;j<len;j++) {
    power += (__real__ x[j]) * (__real__ x[j]) +
        (__imag__ x[j]) * (__imag__ x[j]);
  }
  return power / len;
}

void vec_abs_cf(cf_t *x, float *abs, uint32_t len) {
#ifndef HAVE_VOLK_MAG_FUNCTION
  int i;
  for (i=0;i<len;i++) {
    abs[i] = cabsf(x[i]);
  }
#else
  volk_32fc_magnitude_32f(abs,x,len);

#endif

}

void vec_arg_cf(cf_t *x, float *arg, uint32_t len) {
#ifndef HAVE_VOLK_ATAN_FUNCTION
  int i;
  for (i=0;i<len;i++) {
    arg[i] = cargf(x[i]);
  }
#else
  volk_32fc_s32f_atan2_32f(arg,x,1,len);

#endif

}

uint32_t vec_max_fi(float *x, uint32_t len) {
#ifdef HAVE_VOLK_MAX_FUNCTION
  uint32_t target=0;
  volk_32f_index_max_16u(&target,x,len);
  return target;

#else
  uint32_t i;
  float m=-FLT_MAX;
  uint32_t p=0;
  for (i=0;i<len;i++) {
    if (x[i]>m) {
      m=x[i];
      p=i;
    }
  }
  return p;
#endif
}

void vec_quant_fuc(float *in, unsigned char *out, float gain, float offset, float clip, uint32_t len) {
  int i;
  int tmp;
  for (i=0;i<len;i++) {
    tmp = (int) (offset + gain * in[i]);
    if (tmp < 0)
      tmp = 0;
    if (tmp > clip)
      tmp = clip;
    out[i] = (unsigned char) tmp;
  }

}

