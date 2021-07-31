#define  MAXCHS 8192    /* max number of channels per dimension */

#include "gls.h"
#include "esclev.h"

/* Common Block Declarations */
typedef struct {
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
} Xxgd;
extern Xxgd xxgd;

extern int energy_sum(void);
extern int e_gls_exec(char *ans, int nc);
extern int escl8r_exec(char *ans, int nc);
extern int escl8r_help(void);
extern int e_gls_init(int argc, char **argv);
extern int calc_peaks(void);
extern int chk_fit_igam(int i, int *igam, int *npars, float *saveint, float *saveerr);
extern int chk_fit_egam(int i, int *igam, int *npars, float *saveegam, float *saveerr);
extern int combine(char *ans, int nc); /* returns 1 on error */
extern int curse(void);
extern int cur_gate(void); /* returns 1 on error */
extern int disp_dat(int iflag);
extern int disp_many_sp(void);
extern int energy(float x, float dx, float *eg, float *deg);
extern int eval0(int iy, int ix, float *fit);
extern int eval_fdb(int iy, int ix, float *fit, float *derivs);
extern int eval_fde(int iy, int ix, float *fit, float *derivs);
extern int eval_fdi(int iy, int ix, float *fit, float *derivs);
extern int eval_fwp(int ix, float *fit, float *derivs, int mode);
extern int examine(char *ans, int nc); /* returns 1 on error */
extern int findpks(float spec[6][MAXCHS]);
extern int fitterwp(int npars, int maxits); /* returns 1 on error */
extern int fit_both(void);
extern int fit_egam(void);
extern int fit_igam(void);
extern int fit_width_pars(void);
extern int gate_sum_list(char *ans); /* returns 1 on error */
extern int get_bkgnd(void);
extern int get_cal(void);
extern int get_gate(int ilo, int ihi);
extern int get_list_of_gates(char *ans, int nc, int *outnum, float *outlist,
                             float *wfact); /* returns 1 on error */
extern int get_shapes(void);
extern int multiply(char *ans, int nc); /* returns 1 on error */
extern int num_gate(char *ans, int nc); /* returns 1 on error */
extern int pfind(float *chanx, float *psize, int n1, int n2, int ifwhm,
                 float sigma, int maxpk, int *npk, float spec[6][MAXCHS]);
extern int prep(void);
extern int project(void);
extern int sum_chs(int ilo, int ihi);
extern int sum_cur(void);
extern int write_sp(char *ans, int nc);
extern int wspec2(float *spec, char *filnam, char sp_nam_mod, int expand);
extern float channo(float egamma);
extern float effic(float eg);
