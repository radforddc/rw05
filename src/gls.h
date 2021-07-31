#define MAXLEV  1000    /* max number of levels in scheme */
#define MAXGAM  1500    /* max number of gammas in scheme */
#define MAXBAND 200     /* max number of bands in scheme */
#define MAXTXT  100     /* max number of text labels in scheme */
#define MAXEMIT 30      /* max number of gammas allowed from each level */
#define MAXFIT  500     /* max number of fitted free pars, must be even */
#define CSY     glsgd.csy/glsgd.csx  /* ratio of char height to width */

/* global data */
/* level definition ---------------------- */
typedef struct {      /* DO NOT CHANGE SIZE OR ORDER OF ELEMENTS! */
  float e, de;        /* level energy and energy uncertainty */
  float j, pi;        /* level spin and parity */
  float k;            /* K (projection of spin onto symmetry axis */
  int   band;         /* number of band to which level belongs */
  int   flg;          /* flag for tentative level */
  int   slflg;        /* flag for level spin label */
  int   elflg;        /* flag for level energy label */
  float dxl, dxr;     /* horizontal offsets for stretching/shrinking levels */
                      /* allows level to have different x-edges than those of its band */
  float sldx, sldy;   /* spin label offsets (from default position) in x, y */
  float eldx, eldy;   /* energy label offsets (from default position) in x, y */

  char  name[12];
  float x[4];         /* x-edges for drawing levels */
  char  sl[16];       /* level spin label */
  int   slnc;         /* no. of chars used in sl */
  float slx, sly;     /* level spin label position */
  char  el[16];       /* level energy label */
  int   elnc;         /* no. of chars used in el */
  float elx, ely;     /* level energy label position */
} levdat;

/* gamma definition ---------------------- */
typedef struct {      /* DO NOT CHANGE SIZE OR ORDER OF ELEMENTS! */
  int   li, lf;       /* id numbers of initial, final levels for this gamma */
  char  em;           /* E or M part of multipolarity (Electric/Magnetic) */
  float e, de;        /* gamma energy and energy uncertainty */
  float i, di;        /* gamma intensity and intensity uncertainty */
  float br;           /* branching ratio (not used yet) */
  float a;            /* conversion coefficient */
  float d;            /* mixing ratio */
  float da;           /* conversion coefficient uncertainty */
  float dd;           /* mixing ratio uncertainty */
  float dbr;          /* branching ratio uncertainty (not used yet) */
  float n;            /* lamba = numeric part of multipolarity */
  float x1, x2;       /* x-position of top and bottom of gamma in figure */
  int   flg;          /* flag for tentative gammas */
  int   elflg;        /* flag for gamma energy lebels */
  float eldx, eldy;   /* energy label offsets (from default position) in x, y */

  float x[10], y[10]; /* x, y points for drawing gammas */
  int   np;           /* number of x, y points to use for drawing this gamma */
  char  el[16];       /* gamma energy label */
  int   elnc;         /* no. of chars used in el */
  float elx, ely;     /* gamma energy label position */
} gamdat;

/* band definition ----------------------- */
typedef struct {      /* DO NOT CHANGE SIZE OR ORDER OF ELEMENTS! */
  char  name[8];      /* band name */
  float x0, nx;       /* position and length in x */
  float sldx, sldy;   /* spin label offsets (from default position) in x, y */
  float eldx, eldy;   /* energy label offsets (from default position) in x, y */
  int   sl_onleft;
} bnddat;

/* text label  definition ---------------- */
typedef struct {      /* DO NOT CHANGE SIZE OR ORDER OF ELEMENTS! */
  char  l[40];        /* general-purpose text label */
  int   nc;           /* no. of chars */
  float csx, csy;     /* character size (width, height) in keV units */
  float x, y;         /* label position */
} txtdat;

typedef struct {
  levdat lev[MAXLEV];
  gamdat gam[MAXGAM];
  bnddat bnd[MAXBAND+2];
  txtdat txt[MAXTXT];
  char  gls_file_name[80];
  int   atomic_no, nlevels, ngammas, nbands, ntlabels;
  float csx, csy;                    /* general default character sizes */
  float aspect_ratio, max_tangent, max_dist;   /* parameters for drawing gammas */
  float default_width, default_sep;  /* band width and distance between bands */
  float arrow_width, arrow_length;   /* shape of gamma arrowheads */
  float arrow_break, level_break;    /* break distance in kev for tentative gammas, levels */
  /* character sizes for level spin labels, level energy labels, gamma energy labels: */
  float lsl_csx, lsl_csy, lel_csx, lel_csy, gel_csx, gel_csy;
  float x0,  hix,  y0,  hiy;
  float tx0, thix, ty0, thiy;
  float x00, dx,   y00, dy;
} Glsgd;
extern Glsgd glsgd;

typedef struct {
  int n[MAXLEV];
  int l[MAXLEV][MAXEMIT];
} Levemit;
extern Levemit levemit;

#define MAXSAVESIZE 4096000
typedef struct {
  int  lastmode, prev_pos, next_pos, file_pos, eof;
  int  mark, undo_available, redo_available;
  FILE *file;
} Glsundo;
extern Glsundo glsundo;

/* ==== routines defined in gls.a ==== */
extern int add_gamma(void);
extern int add_gls(void);
extern int add_level(void);
extern int add_text(void);
extern int add_to_band(char *command);
extern int calc_alpha(int igam);
extern int calc_band(void);
extern int calc_gam(int igam);
extern int calc_gls_fig(void);
extern int calc_lev(int ilev);
extern int calc_peaks(void);
extern int change_gamma_label(char *command);
extern int change_level_energy_label(char *command);
extern int change_level_spin_label(char *command);
extern int delete_band(void);
extern int delete_gamma(void);
extern int delete_level(void);
extern int delete_text(void);
extern int display_gls(int iflag);
extern int edit_band(void);
extern int edit_gamma(void);
extern int edit_level(void);
extern int edit_text(void);
extern int examine_gamma(void);
extern int figure_pars(void);
extern int fitterlvls(float *chisq);
extern int fit_lvls(void);
extern float get_energy(void);
extern int get_fit_lev(int *jnl, short *jlev);
extern int gls_exec(char *command);
extern int gls_exec2(char *command);
extern int gls_help(void);
extern int gls_init(char *infilnam);
extern int gls_ps(void);
extern int hilite(int igam);
extern int remove_hilite(int igam);
extern int indices_gls(void);
extern int insert_band(int *iband);
extern int insert_gamma(int *li, int *lf, float *energy, char *ch, int ids, int iband);
extern int insert_level(float *energy, float *spin, int *parity, int iband);
extern int level_width(char *command);
extern int move_band(void);
extern int move_band_gamma(void);
extern int move_band_spin_label(void);
extern int move_energy_label(void);
extern int move_gamma(void);
extern int move_gamma_label(void);
extern int move_level(void);
extern int move_level_spin_label(void);
extern int move_many_bands(void);
extern int move_text(void);
extern int nearest_band(float x, float y);
extern int nearest_gamma(float x, float y);
extern int nearest_level(float x, float y);
extern int open_close_gap(void);
extern int pan_gls(float x, float y, int iflag);
extern int path(int mode);
extern int read_ascii_gls(FILE *file);
extern int read_write_gls(char *command);
extern int remove_band(int iband);
extern int remove_gamma(int igam);
extern int remove_level(int ilev);
extern int swap_gamma(void);
extern int tentative_gamma(void);
extern int tentative_level(void);
extern int testene1(FILE *outfile);
extern int testene2(FILE *outfile);
extern int testint(FILE *outfile);
extern int testsums(void);
extern int up_down_levels(void);
extern int save_gls_now(int mode);
extern int undo_gls(int step);
extern int undo_gls_mark(void);
extern void undo_gls_notify(int flag);
