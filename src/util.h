int cask(char *instring, char *outstring, int maxretlen);
int ask(char *outstring, int maxretlen, const char *fmt, ...);
/* returns number of chars in outstring */
int caskyn(char *instring);
int askyn(const char *fmt, ...);
/* returns 0 for no, 1 for yes */
int askfn(char *ans, int mca,
	  const char *default_fn, const char *default_ext,
	  const char *fmt, ...);
/* returns number of characters received in answer */
int ask_selection(int nsel, int default_sel, const char **choices,
		 const char *flags, const char *head, const char *tail);
/* returns item number selected from among nsel choices (counting from 0)
   default_sel = default choice number (used for simple <rtn>, no choice)
   choices = array of strings from which to choose
   flags = optional array of chars to use as labels for the choices
   head, tail = strings at start and end of question */
void tell(const char *fmt, ...);
void warn(const char *fmt, ...);
void warn1(const char *fmt, ...);
/* returns -1 if no file open, else returns # chars read */
int read_cmd_file(char *outstring, int maxretlen);
/* returns 1 if no log file open, else 0 */
int log_to_cmd_file(char *string);

int datetime(char *dattim);
int ffin(char *instring, int inlength, float *out1, float *out2, float *out3);
/* returns 1 on error */
int file_error(char *inmessage, char *infilnam);
int get_directory(char *env_name, char *dir_name, int maxchar);
int inin(char *instring, int inlength, int *out1, int *out2, int *out3);
/*  returns 1 on error */
int inq_file(char *infilnam, int *outreclen);
/* returns 0 for file not exists, 1 for file exists */
int matread(char *fn, float *sp, char *namesp, int *numch, int idimsp,
	    int *spmode);
/* returns 1 on error */
int matinv(double *array, int order, int dimension);
int matinv_float(float *array, int order, int dimension);
FILE *open_new_file(char *filnam, int force_open);
/* FILE *open_new_unf(char *filnam); DEPRECATED
   FILE *open_pr_file(char *filnam); DEPRECATED */
FILE *open_save_file(char *filename, int save_rename);
FILE *open_readonly(char *filnam);
/* return valid file descriptor on success, NULL on failure */
int pr_and_del_file(char *filnam);
int read_cal_file(char *infilnam, char *outtitle, int *outorder,
		  double *outpars);
/* returns 1 on error */
int read_eff_file(char *infilnam, char *outtitle, float *outpars);
/* returns 1 on error */
int read_spe_file(char *infilnam, float *outspec,
		  char *outnam, int *outnumchs, int inmaxchs);
/* returns 1 on error */
int read_tab_file(char *infilnam, int *outnclook, int *outlookmin,
		  int *outlookmax, short *outlooktab, int inmaxchs);
/* returns 1 on error */
int readsp(char *filnam, float *sp, char *namesp, int *numch, int idimsp);
/* returns 1 on error */
int rmat(FILE *fd, int ich0, int nchs, short *matseg);
int rmat4b(FILE *fd, int ich0, int nchs, int *matseg);
int wmat(FILE *fd, int ich0, int nchs, short *matseg);
int wmat4b(FILE *fd, int ich0, int nchs, int *matseg);
int setext(char *filnam, const char *cext, int filnam_len);
int spkio(int mode, FILE *spkfile, int idn, int *ihed, int maxh,
	  int *idat, int *ndx, int nch);
/* mode = 0,1,2 says initialize, read, write
   mode = 6 says return id-list in idat (idat[0] = # of id's)
   spkfile = file descr. of open file
   idn  = requested id #
   ihed - array to contain header
   maxh = maximum length of ihed in half-words
   idat - array to contain data
   ndx  - array to contain indices of 1st channel to xfer
   nch  = # of channels to xfer
   returns 0 on success, error code on failure */
FILE *spkman(char *mode, char *namf, char *iacp);
/* open/create spk-files for input/output
   mode = "OPEN", "CREA"
   namf = filename
   iacp = "RO", "RW" = access mode
   returns valid file descriptor on success, NULL on failure */
int spkread(char *fn, float *sp, char *namesp, int *numch, int idimsp);
/* returns 1 on error */
int wspec(char *filnam, float *spec, int idim);
int wrresult(char *out, float value, float err, int minlen);

void swapb8(char *buf);
void swapb4(char *buf);
void swapb2(char *buf);
int get_file_rec(FILE *fd, void *data, int maxbytes, int swap_bytes);
int put_file_rec(FILE *fd, void *data, int numbytes);
