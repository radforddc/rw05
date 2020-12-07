#define  MAXCHS 1400    /* max number of channels per dimension */
                        /*    (must be multiple of 8)    */

#include "gls.h"
#include "esclev.h"

/* Common Block Declarations */
struct {
  int   numchs;
  float spec[6][MAXCHS], old_spec[6][MAXCHS], bspec[6][MAXCHS];
  float v_depth[MAXCHS], v_width;
  char  name_gat[80], old_name_gat[80];  /* changed from 20 or 30 */

  int   lo_ch[MAXGAM], hi_ch[MAXGAM];
  int   npks_ch[MAXCHS];                     /* no. of gammas for each ch. */
  short pks_ch[MAXCHS][60];                  /* gammas w/ counts in ch. */
  int   fitchx[MAXCHS], fitchy[MAXCHS];

  char  cubnam[80], levfile[80], fdname[80], fdgfile[80];
  char  progname[8];

  float eff_sp[MAXCHS], energy_sp[MAXCHS];
  float elo_sp[MAXCHS], ehi_sp[MAXCHS], ewid_sp[MAXCHS];

  int   luch[MAXCHS+1], matchs ,le2pro2d;
  float pro2d[MAXCHS*(MAXCHS+1)/2];
  float e2pro2d[MAXCHS*(MAXCHS+1)/2], e2e2spec[MAXCHS], e2e2e2sum;

  float bf1, bf2, bf4, bf5;

  float pk_shape[MAXGAM][15];
  float pk_deriv[MAXGAM][15];
  float w_deriv [MAXGAM][15];

  short looktab[16384];
  int   nclook, lookmin, lookmax;

  /* stuff added for linear combinations of cubes: */
  float dpro2d[MAXCHS*(MAXCHS+1)/2];
  char  cubenam1[5][80];
  float cubefact[5];
  int   many_cubes;
} xxgd;

/* from lev4d.c */
int l_gls_exec(char *ans, int nc);
int lvt8r_exec(char *ans, int nc);
int lvt8r_help(void);
int check_band(char *ans, int nc);
int energy(float x, float dx, float *eg, float *deg);
int energy_sum(void);
int examine_trip(void);
int add_gate(float elo, float ehi);
int add_trip_gate(float elo1, float ehi1, float elo2, float ehi2);
int calc_peaks(void);
int chk_fit_igam(int i, int *igam, int *npars, float *saveint,
		 float *saveerr, int fit_trip);
int chk_fit_egam(int i, int *igam, int *npars, float *saveegam, float *saveerr);
int curse(void);
int cur_gate(void); /* returns 1 on error */
int disp_dat(int iflag);
int disp_many_sp(void);
int energy_nonlin(float x, float dx, float *eg, float *deg);
int eval0(int iy, int ix, float *fit);
int eval_fdb(int iy, int ix, float *fit, float *derivs);
int eval_fde(int iy, int ix, float *fit, float *derivs);
int eval_fdi(int iy, int ix, float *fit, float *derivs);
int eval0_t(int iz, int iy, int ix, float *fit);
int eval_fdb_t(int iz, int iy, int ix, float *fit, float *derivs);
int eval_fde_t(int iz, int iy, int ix, float *fit, float *derivs);
int eval_fdi_t(int iz, int iy, int ix, float *fit, float *derivs);
int eval_fwp(int ix, float *fit, float *derivs, int mode);
int examine(char *ans, int nc); /* returns 1 on error */
int findpks(float spec[6][MAXCHS]);
int fitterwp(int npars, int maxits); /* returns 1 on error */
int fit_both(int fit_trip);
int fit_egam(int fit_trip);
int fit_igam(int fit_trip);
int fit_width_pars(void);
int gate_sum_list(char *ans); /* returns 1 on error */
int get_bkgnd(void);
int get_cal(void);
int get_gate(float elo, float ehi);
int get_list_of_gates(char *ans, int nc, int *outnum, float *outlist,
		      float *wfact); /* returns 1 on error */
int get_lookup_table(void);
int get_shapes(void);
int multiply(char *ans, int nc); /* returns 1 on error */
int num_gate(char *ans, int nc); /* returns 1 on error */
int pfind(float *chanx, float *psize, int n1, int n2, int ifwhm,
	  float sigma, int maxpk, int *npk, float spec[6][MAXCHS]);
int project(void);
int read_cube(int ilo1, int ihi1, int ilo2, int ihi2);
int sum_eng(float elo, float ehi);
int sum_cur(void);
int trip_gate(char *ans, int nc); /* returns 1 on error */
int write_sp(char *ans, int nc);
int wspec2(float *spec, char *filnam, char sp_nam_mod, int expand);
int ichanno(float egamma);
float effic(float eg);

/* from levit8ra.c, 4dg8ra.c */
int l_gls_init(int argc, char **argv);
int combine(char *ans, int nc); /* returns 1 on error */
int prep(void);
int read_write_l4d_file(char *command);
int fdg8r_exec(char *ans, int nc);
int fdg8r_help(void);
int fdg8r_init(int argc, char **argv);
int examine_quad(void);
int add_quad_gate(float elo1, float ehi1, float elo2, float ehi2,
		  float elo3, float ehi3);
int calc_4d_spec(int ilo1, int ihi1, int ilo2, int ihi2, int ilo3, int ihi3,
		 float *spec1, float *spec2);
int check_4d_band(char *ans, int nc);
int prep4dg(void);
int quad_gate(char *ans, int nc); /* returns 1 on error */
int open3dfile(char *name, int *numchs_ret); /* returns 1 on error */
int open4dfile(char *name, int *numchs_ret); /* returns 1 on error */
int close3dfile(void);
int close4dfile(void);
int read3dcube(int lo1, int hi1, int lo2, int hi2, int *spec_ret);
int read4dcube(int lo1, int hi1, int lo2, int hi2, int lo3, int hi3,
	       int *spec_ret);
int sum4gate(int *lo, int *hi);
int setcubenum(int cubenum);
