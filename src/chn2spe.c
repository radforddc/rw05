#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>


struct spe_header{
  int reclA;            /* 24 */
  char title[8];
  int dim;
  int a1;               /*  1 */
  int a2;               /*  1 */
  int a3;               /*  1 */
  int reclB;            /* 24 */
};

/*write_spectrum
 *
 * saves the spectrum pointed to by "spec". Returns 0 if unsuccessful,
 * 1 if successful.
 * The name is truncated by removing any trailing .spe (and any
 * subsequent characters...), as well as any leading directory names.
 * If the resulting string is longer than the maximum allowed 8
 * characters, only the first 8 are retained
 */
static int write_spectrum(float *spec, int nchs, char *name) {  
  FILE *fp;
  int record_length;
  struct spe_header header;

  header.reclA = header.reclB = 24; 
  header.title[0] = header.title[1] = 0;
  header.a1 = header.a2 = header.a3 = 1;

  fp = fopen(name,"w");
  if (fp == NULL){
    fprintf(stderr,"Error! Unable to open spectrum file %s \n",name);
    return 0;
  }
  header.dim = nchs;
  memcpy(header.title, name, 8);
  record_length = sizeof(float)*header.dim;

  fwrite(&header, sizeof(header), 1, fp);
  fwrite(&record_length, sizeof(record_length), 1, fp);
  fwrite(spec, sizeof(float), nchs, fp); 
  fwrite(&record_length, sizeof(record_length), 1,fp);
  fclose(fp);

  return 1;
}

int main(int argc, char **argv) {

  int    i, nchs=0;
  short  s;
  char   fin[256], fout[256],line[120], *c;
  float  spec[16384];
  int    isp[16384] = {0};
  float  a[3] = {0};
  FILE  *fp;

  if (argc < 2) {
    printf("No file name provided...\n");
    return 1;
  }
  
  strncpy(fin, argv[1], sizeof(fin));
  if ((fp = fopen(fin, "r")) == NULL) {
    printf("Cannot open input file %s\n", fin);
    return 1;
  }
  // skip first 30 bytes of file (header)
  fread(line, 30, 1, fp);
  // read number of channels in spectrum
  fread(&s, 2, 1, fp);
  nchs = s;
  if (nchs > 16384) nchs = 16384;
  if (nchs < 4) {
    printf("Cannot read input file %s\n", fin);
    return 1;
  }

  /* read data */
  fread(isp, 4, nchs, fp);

  /* read calibration */
  fseek(fp, 4*s+32, SEEK_SET);
  fread(&s, 2, 1, fp);
  if (s != -101 && s != -102) {
    printf("Hmmm... cannot read cakibvration. s = %d\n", s);
    s = 0;
  } else {
    fread(line, 2, 1, fp);  // skip reserved bytes
    fread(a, 4, 3, fp);     // energy calibration coefficients
    if (s == -101) a[2] = 0;
  }

  fclose(fp);
  
  printf("%i channels...\n", nchs);
  if (nchs < 4) {
    printf("Cannot read input file %s\n", fin);
    return 1;
  }

  /* write .spe file */
  for (i=0; i<nchs; i++) spec[i] = isp[i];
  strncpy(fout, fin, sizeof(fout));
  if ((c = strstr(fout, ".CHN")) == NULL &&
      (c = strstr(fout, ".Chn")) == NULL &&
      (c = strstr(fout, ".chn")) == NULL) c = fout + strlen(fout);
  strncpy(c, ".spe", 4);
  write_spectrum(spec, nchs, fout);
  printf("  %s ==> %s\n", fin, fout);

  /* write .aca file */
  if (s) {
    if (s == -102 && a[2] == 0.0) s = -101;
    strncpy(c, ".aca", 4);
    if ((fp = fopen(fout, "w")) == NULL) return 1;
    fprintf(fp, "  ENCAL OUTPUT FILE\n From %s\n", fin);
    fprintf(fp, "%5d %18.11E %18.11E %18.11E\n"
            "      %18.11E %18.11E %18.11E\n", -100 - s,
            a[0], a[1], a[2], 0.0, 0.0, 0.0);
    fclose(fp);
    printf("   ...Calibration file %s created. %d terms\n", fout, -99 - s);
    fclose(fp);
  }

  return 0;
}
