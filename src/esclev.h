#ifndef _ESCLEV_H
#define _ESCLEV_H      1

#define MAXFIT 500     /* max number of free parameters in fits */

/* Common Block Declarations */
typedef struct {
  float bg_err, encal_err, effcal_err;
  float finest[5], swpars[3], h_norm, t_norm, q_norm;
  float min_ny, multy;
  int   pkfind, isigma, ipercent, disp_calc;
  int   n_stored_sp;
  int   loch, hich, ngd, locnt, nchs, ncnts, iyaxis, lox, numx;
  int   colormap[15];

  int   nlist[55];
  float list[55][40];

  short nfitdat[MAXGAM], ifitdat[MAXGAM][MAXGAM];
  float hpk[2][MAXGAM];

  float levelbr[MAXGAM][MAXLEV];

  int   gotsignal;
} Elgd;
extern Elgd elgd;

typedef struct {
  int   npars, nepars, nipars, idat[MAXFIT], idat2[MAXFIT] ;
  float savedat[MAXFIT],  saveerr[MAXFIT];
  float savedat2[MAXFIT], saveerr2[MAXFIT];
  float alpha[MAXFIT][MAXFIT];
  float beta[MAXFIT], pars[MAXFIT], derivs[MAXFIT], delta[MAXFIT], ers[MAXFIT];
  int   nextp[MAXFIT];
} Fgd;
extern Fgd fgd;

/* these are from minig / gls_minig */
extern float fdx, fx0, fdy, fy0;
extern int   idx, ix0, idy, iy0, iyflag;

extern int  rlen;
extern char lx[120], prfilnam[80];
extern FILE *file, *prfile, *filez[40];

extern void breakhandler(int dummy);
extern int get_fit_gam(int *jng, short *jgam);
extern int fitter2d(char mode, int maxits);
extern int fitter3d(char mode, int maxits);
extern int listgam(void);
extern int comfil(char *filnam, int nc);
extern int wrlists(char *ans);
extern int save_esclev_now(int mode);
extern int undo_esclev(int step, int mode);
extern int rw_saved_spectra(char *ans);
extern void set_gate_label_h_offset(int offset);

extern void set_signal_alert(int mode, char *mesag);
extern int check_signal_alert(void);

/* ==== other external routines ==== */
extern int initg2(int *, int *);
extern int retic2(float *, float *, char *);
extern int retic3(float *, float *, char *, int *);

#endif /* esclev.h  */
