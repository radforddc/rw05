#include <stdio.h>
#include <stdlib.h>
#include <math.h>


int
compress (unsigned short *in, unsigned char *out)
{
  static int sum_limits[] = { 16,32,64,128,256,512,1024,2048,4096,
                  8192,17000,35000,80000,170000 };

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

  if (sum > sum_limits[8]) {
    if (sum > sum_limits[12]) {
      if (sum > sum_limits[13]) {
	nplanes = 13;
      } else {
	nplanes = 12;
      }
    } else if (sum > sum_limits[10]) {
      if (sum > sum_limits[11]) {
	nplanes = 11;
      } else {
	nplanes = 10;
      }
    } else if (sum > sum_limits[9]) {
      nplanes = 9;
    } else {
      nplanes = 8;
    }
  } else if (sum > sum_limits[4]) {
    if (sum > sum_limits[6]) {
      if (sum > sum_limits[7]) {
	nplanes = 7;
      } else {
	nplanes = 6;
      }
    } else if (sum > sum_limits[5]) {
      nplanes = 5;
    } else {
      nplanes = 4;
    }
  } else if (sum > sum_limits[2]) {
    if (sum > sum_limits[3]) {
      nplanes = 3;
    } else {
      nplanes = 2;
    }
  } else if (sum > sum_limits[1]) {
    nplanes = 1;
  } else {
    nplanes = 0;
  }

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
    } else if (test1 > 31) {
      nplanes += 1;
      noverflows = test2;
      break;
    }
    noverflows = test1;
    break;
  }

  if (nplanes > 15)
    fprintf(stderr,"Expecting core dump...\n");

  /* insert length of compressed cube */
  if (nplanes < 7) {
    out[0] = 32*nplanes + noverflows;
    bitptr = out+1;
    cube_length = 1;
  } else {
    out[0] = 224 + noverflows;
    out[1] = nplanes-7;
    bitptr = out+2;
    cube_length = 2;
  }
  ovrptr = bitptr + nplanes*32;
  cube_length += nplanes*32 + noverflows;

  /* now, compress */
  /* prepare bit planes and insert overflow bits... */
  if (nplanes > 0) {
    stmp = 0;
    nbits = 0;
    for (i=0;i<256;i++) {
      /* insert nplanes number of bits */
      stmp = (stmp << nplanes) + (in[i] & bitmask[nplanes]);
      nbits += nplanes;
      while (nbits > 7) {
	/* printf("nplanes i nbits: %d %d %d\n", nplanes, i, nbits); */
        nbits -= 8;
        *bitptr++ = stmp >> nbits;
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
      for(j=0; j<in[i]; j++)
	*ovrptr++ = i;
    }
  }

  return cube_length;
}

void
decompress (unsigned char in[512], unsigned short out[256])
{
  static int bitmask[] = { 0,1,3,7,15,31,63,127,255,511,1023,2047,
               4095,8191,16383,32767 };
  int nplanes, noverflows, nbits;
  unsigned char *bitptr;
  int i, t;

  nplanes = in[0] >> 5;
  noverflows = in[0] & 31;
  if (nplanes == 7) {
    nplanes += in[1];
    bitptr = in+2;
  } else {
    bitptr = in+1;
  }
  /* printf("nplanes, noverflows, data: %d %d %d %d %d\n",
     nplanes, noverflows, in[0], in[1], in[2]); */

  /* extract bit planes */
  if (nplanes > 0) {
    nbits = 0;
    for (i=0;i<256;i++) {
      if (nbits+nplanes < 9) {
        nbits += nplanes;
        out[i] = (*bitptr >> (8-nbits)) & bitmask[nplanes];
        if (nbits > 7) {
          bitptr++;
          nbits = 0;
        }
      } else {
        t = nplanes-8+nbits;
        out[i] = ((*bitptr++ & bitmask[8-nbits]) << t);
	while (t>7) {
	  t -= 8;
	  out[i] += (*bitptr++ << t);
	}
        if (t>0) out[i] += (*bitptr >> (8-t));
        nbits = t;
      }
    }
  } else {
    for (i=0; i<256; i++) {
      out[i] = 0;
    }
  }

  /* extract overflows */
  for (i=0;i<noverflows;i++) {
    out[*bitptr++] += 1 << nplanes;
  }
}
