#define  MAXCHS 4096    /* max number of channels per dimension */

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

  double gain[6];
  int    nterms;
  float  eff_pars[9];

  int    esc_file_flags[10];
  float  rdata[MAXCHS], rdata2[MAXCHS];

  float  pk_shape[MAXGAM][40];
  float  pk_deriv[MAXGAM][40];
  float  w_deriv [MAXGAM][40];
} xxgd;

int energy_sum(void);
int e_gls_exec(char *ans, int nc);
int escl8r_exec(char *ans, int nc);
int escl8r_help(void);
int e_gls_init(int argc, char **argv);
int calc_peaks(void);
int chk_fit_igam(int i, int *igam, int *npars, float *saveint, float *saveerr);
int chk_fit_egam(int i, int *igam, int *npars, float *saveegam, float *saveerr);
int combine(char *ans, int nc); /* returns 1 on error */
int curse(void);
int cur_gate(void); /* returns 1 on error */
int disp_dat(int iflag);
int disp_many_sp(void);
int energy(float x, float dx, float *eg, float *deg);
int eval0(int iy, int ix, float *fit);
int eval_fdb(int iy, int ix, float *fit, float *derivs);
int eval_fde(int iy, int ix, float *fit, float *derivs);
int eval_fdi(int iy, int ix, float *fit, float *derivs);
int eval_fwp(int ix, float *fit, float *derivs, int mode);
int examine(char *ans, int nc); /* returns 1 on error */
int findpks(float spec[6][MAXCHS]);
int fitterwp(int npars, int maxits); /* returns 1 on error */
int fit_both(void);
int fit_egam(void);
int fit_igam(void);
int fit_width_pars(void);
int gate_sum_list(char *ans); /* returns 1 on error */
int get_bkgnd(void);
int get_cal(void);
int get_gate(int ilo, int ihi);
int get_list_of_gates(char *ans, int nc, int *outnum, float *outlist,
		      float *wfact); /* returns 1 on error */
int get_shapes(void);
int multiply(char *ans, int nc); /* returns 1 on error */
int num_gate(char *ans, int nc); /* returns 1 on error */
int pfind(float *chanx, float *psize, int n1, int n2, int ifwhm,
	  float sigma, int maxpk, int *npk, float spec[6][MAXCHS]);
int prep(void);
int project(void);
int sum_chs(int ilo, int ihi);
int sum_cur(void);
int write_sp(char *ans, int nc);
int wspec2(float *spec, char *filnam, char sp_nam_mod, int expand);
float channo(float egamma);
float effic(float eg);
