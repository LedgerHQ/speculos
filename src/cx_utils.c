#include "cx_utils.h"
/* ======================================================================= */
/*                          32 BITS manipulation                           */
/* ======================================================================= */
#ifndef CX_INLINE_U32
// --------------------------------------------------------------------------
// -
// --------------------------------------------------------------------------
unsigned long int cx_rotl(unsigned long int x, unsigned char n) {
  return   (((x)<<(n)) | ((x)>>(32-(n))));
}

// --------------------------------------------------------------------------
// -
// --------------------------------------------------------------------------
unsigned long int cx_rotr(unsigned long int x, unsigned char n) {
  return  ( ((x) >> (n)) | ((x) << ((32) - (n))) );
}

// --------------------------------------------------------------------------
// -
// --------------------------------------------------------------------------
unsigned long int cx_shr(unsigned long int x, unsigned char n) {
   return ( (x) >> (n) );
}
#endif

// --------------------------------------------------------------------------
// -
// --------------------------------------------------------------------------
uint32_t cx_swap_uint32(uint32_t v) {
  return 
    ( ((v)<<24) & 0xFF000000U ) |
    ( ((v)<<8)  & 0x00FF0000U ) |
    ( ((v)>>8)  & 0x0000FF00U ) |
    ( ((v)>>24) & 0x000000FFU );
}

// --------------------------------------------------------------------------
// -
// --------------------------------------------------------------------------
void cx_swap_buffer32(uint32_t *v, size_t len) {
  while (len--)  {
    #ifdef ST31
    unsigned int tmp;
    tmp = ((unsigned char*)v)[len*4+3];
    ((unsigned char*)v)[len*4+3]=((unsigned char*)v)[len*4];
    ((unsigned char*)v)[len*4]  = tmp;
    tmp = ((unsigned char*)v)[len*4+2];
    ((unsigned char*)v)[len*4+2]=((unsigned char*)v)[len*4+1];
    ((unsigned char*)v)[len*4+1]=tmp;
    #else
    v[len] = cx_swap_uint32(v[len]);
    #endif
  }
}

/* ======================================================================= */
/*                          64 BITS manipulation                           */
/* ======================================================================= */
#ifndef NATIVE_64BITS //NO 64BITS
// --------------------------------------------------------------------------
// -
// --------------------------------------------------------------------------
 void cx_rotr64(uint64bits_t *x, unsigned int n) {
  unsigned long int  sl_rot, sh_rot;
  if (n >= 32) {
    sl_rot = x->l;
    x->l = x->h;
    x->h = sl_rot;
    n -= 32;
  } 
  sh_rot = ((((x->h)&0xFFFFFFFF)<<(32-n)));
  sl_rot = ((((x->l)&0xFFFFFFFF)<<(32-n)));
  //rotate
  x->h     = ((x->h >>n) |sl_rot);
  x->l     = ((x->l >>n) |sh_rot);
}
// --------------------------------------------------------------------------
// -
// --------------------------------------------------------------------------
void cx_shr64(uint64bits_t *x, unsigned char n) {
 unsigned long int  sl_shr;    

  if (n >= 32) {
    x->l = (x->h);
    x->h = 0;
    n -= 32;
  } 

  sl_shr = ((((x->h)&0xFFFFFFFF)<<(32-n)));
  x->l = ((x->l)>>n)|sl_shr;
  x->h = ((x->h)>>n);

}

#else

#ifndef CX_INLINE_U64
uint64bits_t cx_rotr64(uint64bits_t x, unsigned int n) {
 return  ( ((x) >> (n)) | ((x) << ((64) - (n))) );
}
uint64bits_t cx_rotl64(uint64bits_t x, unsigned int n) {
 return  ( ((x) << (n)) | ((x) >> ((64) - (n))) );
}
uint64bits_t cx_shr64(uint64bits_t x, unsigned int n) {
  return ( (x) >> (n) ); 
}
#endif

#endif

// --------------------------------------------------------------------------
// -
// --------------------------------------------------------------------------

#ifndef NATIVE_64BITS
void cx_swap_uint64(uint64bits_t *v) {
  unsigned long int h,l;
  h = v->h;
  l = v->l;  
  l = cx_swap_uint32(l);
  h = cx_swap_uint32(h);
  v->h = l;
  v->l = h;
}
#else // HAVE_SYS_UINT64_SUPPORT
uint64bits_t  cx_swap_uint64(uint64bits_t v) {
  uint32_t h,l;
  h = (uint32_t)((v>>32) & 0xFFFFFFFF);
  l = (uint32_t)(v & 0xFFFFFFFF);
  l = cx_swap_uint32(l);
  h = cx_swap_uint32(h);
  return   (((uint64bits_t)l)<<32) | ((uint64bits_t)h);
}
#endif // HAVE_SYS_UINT64_SUPPORT

// --------------------------------------------------------------------------
// -
// --------------------------------------------------------------------------
void cx_swap_buffer64(uint64bits_t* v, int len) {
#ifndef  NATIVE_64BITS
  while(len--) {
    cx_swap_uint64(&v[len]);
  }
#else // HAVE_SYS_UINT64_SUPPORT
    uint64bits_t i;
    while(len--) {
      i = *v;
      *v++ = cx_swap_uint64(i);
    }
#endif // HAVE_SYS_UINT64_SUPPORT

}

// --------------------------------------------------------------------------
// -
// --------------------------------------------------------------------------
#ifndef  NATIVE_64BITS
void cx_add_64(uint64bits_t *x,uint64bits_t *y) {
  unsigned int      carry;
  unsigned long int addl;

  addl = x->l+y->l;
  addl = ~addl;
  carry = ((((x->l & y->l) | (x->l & ~y->l & addl) | (~x->l & y->l & addl)) >> 31) & 1);        

  x->l = x->l + y->l;
  x->h = x->h + y->h;
  if (carry) {
    x->h++;
  }
}
#endif
