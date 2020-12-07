#include <stdio.h>
#include <stdlib.h>
#include <math.h>


int
compress3d (unsigned int *in, unsigned char *out)
{
  static int sum_limits[] = { 16,32,64,128,
                  256,512,1024,2048,
                  4096,8192,17000,35000,
                  80000,160000,320000,640000 };

  static int bitmask[] = { 0,1,3,7,15,31,63,127,255,511,1023,2047,
               4095,8191,16383,32767 };
  int cube_length;
  int sum=0, i, j;
  int nplanes, noverflows, stmp, nbits;
  unsigned char *bitptr, *ovrptr;
  int test1, test2, test3;

  /* guess at depth of bit field */
  for (i=0;i<256;i+=16) {
    sum += in[i];
  }

  i = 7;
  j = 8;
  while (j>1) {
    j = j / 2;
    if (sum > sum_limits[i])
      i += j;
    else
      i -= j;
  }
  if (sum > sum_limits[i+1])
    nplanes = i + 1;
  else
    nplanes = i;

  while (1) {
    test1 = test2 = test3 = 0;
    for (i=0;i<256;i++) {
      test1 += in[i] >> nplanes;
      test2 += in[i] >> (nplanes+1);
      test3 += in[i] >> (nplanes+2);
    }
    if (test2 > 31) {
      if (test3 > 31) {
	nplanes += 3;
	continue;
      }
      nplanes += 2;
      noverflows = test3;
      break;
    }
    else if (test1 > 31) {
      nplanes += 1;
      noverflows = test2;
      break;
    }
    noverflows = test1;
    break;
  }

  if (nplanes > 30)
    fprintf(stderr,"Expecting core dump...\n");

  /* insert length of compressed cube */
  if (nplanes < 7) {
    out[0] = 32*nplanes + noverflows;
    bitptr = out+1;
    cube_length = 1;
  }
  else {
    out[0] = 224 + noverflows;
    out[1] = nplanes-7;
    bitptr = out+2;
    cube_length = 2;
  }
  ovrptr = bitptr + nplanes*32;
  cube_length += nplanes*32 + noverflows;

  /* now, compress */
  /* prepare bit planes and insert overflow bits... */
  while (nplanes>=8) {
    for (i=0; i<256; i++) {
      *bitptr++ = in[i]&bitmask[8];
      in[i] = in[i]>>8;
    }
    nplanes -= 8;
  }

  if (nplanes > 0) {
    stmp = 0;
    nbits = 0;
    for (i=0; i<256; i++) {
      /* insert nplanes number of bits */
      stmp = (stmp << nplanes) + (in[i] & bitmask[nplanes]);
      nbits += nplanes;
      if (nbits > 7) {
        *bitptr++ = stmp >> (nbits - 8);
        nbits -= 8;
        stmp &= bitmask[nbits];
      }

      /* append overflows */
      noverflows = in[i] >> nplanes;
      for(j=0; j<noverflows; j++)
	*ovrptr++ = i;
    }
  }
  else { /* just do overflows */
    for (i=0; i<256; i++) {
      for(j=0; j<in[i]; j++) {
      *ovrptr++ = i;
      }
    }
  }

  return cube_length;
}

void
decompress3d (unsigned char in[1024], unsigned int out[256])
{
  static int bitmask[] = { 0,1,3,7,15,31,63,127,255,511,1023,2047,
               4095,8191,16383,32767 };
  int nplanes, noverflows, nbits, savenplanes;
  unsigned char *bitptr;
  int i, j, t;

  nplanes = in[0] >> 5;
  noverflows = in[0] & 31;
  if (nplanes == 7) {
    nplanes += in[1];
    bitptr = in+2;
  }
  else {
    bitptr = in+1;
  }
  /* printf("%d %d %d\n",nplanes,noverflows,*bitptr); */

  /* extract bit planes */
  savenplanes = nplanes;
  for (i=0; i<256; i++)
    out[i] = 0;
  j = 0;
  while (nplanes>=8) {
    for (i=0; i<256; i++)
      out[i] += ((unsigned int)*bitptr++)<<j;
    nplanes -= 8;
    j += 8;
  }

  if (nplanes > 0) {
    nbits = 0;
    for (i=0; i<256; i++) {
      if (nbits+nplanes < 9) {
        out[i] += ((*bitptr >> (8-nbits-nplanes)) & bitmask[nplanes])<<j;
        nbits += nplanes;
        if (nbits > 7) {
          bitptr++;
          nbits -= 8;
        }
      }
      else {
        t = nplanes-8+nbits;
        out[i] += (((*bitptr & bitmask[8-nbits]) << t)
	           + (*(bitptr+1) >> (8-t)))<<j;
	bitptr++;
        nbits = t;
      }
    }
  }

  /* extract overflows */
  for (i=0; i<noverflows; i++)
    out[*bitptr++] += 1 << savenplanes;

}
