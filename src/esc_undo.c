#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "escl8r.h"
#include "util.h"

typedef struct {
  double gain[6];
  int    nterms;
  float  eff_pars[9];
  int    esc_file_flags[10];
  float  bg_err, encal_err, effcal_err;
  float  finest[5], swpars[3], h_norm, t_norm, q_norm;
  float  bspec[6][MAXCHS];
} undo_esc_data;

typedef struct {
  float  spec[6][MAXCHS];
  char   name_gat[80];
} undo_gate_data;

struct {
  undo_esc_data  esc[10];
  undo_gate_data gates[20];
  int  max_esc, max_gate;
  int  pos_esc, pos_gate;
  int  min_esc, min_gate;
} esclev_undo;
int esclev_undo_ready = 0;

#ifdef GTK
void set_gate_button_sens(int sens);
#endif

/* ======================================================================= */
int save_esclev_now(int mode)
{
  /*
    save current gates and data file info to temp file
    mode = -1 to reset (e.g. when new file opened)
    mode =  1 to save coincidence data file info (calibs etc.)
            3 to save projections & background spectra
            4 to save gates
  */

  int i;

  if (!esclev_undo_ready) {
    esclev_undo.max_esc = esclev_undo.max_gate = -1;
    esclev_undo.pos_esc = esclev_undo.pos_gate = -1;
    esclev_undo.min_esc = 0;
    esclev_undo.min_gate = 2;
    esclev_undo_ready = 1;
  }

  if (mode < 0) {
    esclev_undo.max_esc = esclev_undo.max_gate = -1;
    esclev_undo.pos_esc = esclev_undo.pos_gate = -1;
    esclev_undo.min_esc = 0;
    esclev_undo.min_gate = 1;
    return 0;
  }

  if (mode < 4) {
    i = (++esclev_undo.pos_esc) % 10;
    memcpy(&esclev_undo.esc[i].gain[0], &xxgd.gain[0], 6*8 + 4 + 9*4 + 10*4);
    memcpy(&esclev_undo.esc[i].bg_err,  &elgd.bg_err,  14*4);
    memcpy(&esclev_undo.esc[i].bspec[0][0], &xxgd.bspec[0][0], 24*MAXCHS);
    esclev_undo.max_esc = esclev_undo.pos_esc;
    if (esclev_undo.min_esc < esclev_undo.pos_esc - 9)
      esclev_undo.min_esc = esclev_undo.pos_esc - 9;
  } else {
    if (esclev_undo.pos_gate < 0) {
      i = (++esclev_undo.pos_gate) % 20;
      memcpy(&esclev_undo.gates[i].spec[0][0], &xxgd.old_spec[0][0], 24*MAXCHS);
      memcpy(&esclev_undo.gates[i].name_gat[0], &xxgd.old_name_gat[0], 80);
    }
    i = (++esclev_undo.pos_gate) % 20;
    memcpy(&esclev_undo.gates[i].spec[0][0], &xxgd.spec[0][0], 24*MAXCHS);
    memcpy(&esclev_undo.gates[i].name_gat[0], &xxgd.name_gat[0], 80);
    esclev_undo.max_gate = esclev_undo.pos_gate;
    if (esclev_undo.min_gate < esclev_undo.pos_gate - 19)
      esclev_undo.min_gate = esclev_undo.pos_gate - 19;

#ifdef GTK
    if (esclev_undo.pos_gate > esclev_undo.min_gate) set_gate_button_sens(1);
    set_gate_button_sens(-2);
#endif
  }
  return 0;
} /* save_esclev_now */

/* ======================================================================= */
int undo_esclev(int step, int mode)
{
  /* undo or redo esclev file/gate edits */
  int  i, reclen = xxgd.numchs*4;
  char dattim[20];

  if (!esclev_undo_ready || step == 0) return 1;

  /* first check to see that we have gates/changes to undo or redo */
  if (step < 0 && 
      ((mode < 4  && esclev_undo.pos_esc < esclev_undo.min_esc) ||
       (mode >= 4 && esclev_undo.pos_gate < esclev_undo.min_gate))) {
    tell("%cNo more undo information...\n", (char) 7);
    return 1;
  }
  if (step > 0 &&
      ((mode < 4  && esclev_undo.pos_esc >= esclev_undo.max_esc - 1) ||
       (mode >= 4 && esclev_undo.pos_gate >= esclev_undo.max_gate - 1))) {
    tell("%cNo more redo information...\n", (char) 7);
    return 1;
  }

  if (mode < 4) {
    /* deal with changes to the .esc file, not the gates */
    if (step < 0) {
      /* undo previous edits */
      /*  first check to see if we need to save the _current_ data
	  for possible recall later */
      if (esclev_undo.pos_esc == esclev_undo.max_esc) {
	save_esclev_now(mode);
	esclev_undo.pos_esc--;
      }
      i = (esclev_undo.pos_esc--) % 10;
    } else {
      /* redo previously undone edits */
      i = (++esclev_undo.pos_esc + 1) % 10;
    }
    memcpy(&xxgd.gain[0], &esclev_undo.esc[i].gain[0], 6*8 + 4 + 9*4 + 10*4);
    memcpy(&elgd.bg_err,  &esclev_undo.esc[i].bg_err,  14*4);
    memcpy(&xxgd.bspec[0][0], &esclev_undo.esc[i].bspec[0][0], 24*MAXCHS);

    /* log the undo/redo to the .esc_log file */
    datetime(dattim);
    if (step < 0) {
      fprintf(filez[26], "%s: Undo!\n", dattim);
    } else {
      fprintf(filez[26], "%s: Redo!\n", dattim);
    }
    fflush(filez[26]);
    /* update the .esc file */
    fseek(filez[2], (xxgd.numchs + 6)*reclen, SEEK_SET);
    fwrite(&xxgd.gain[0], 8, 6, filez[2]);
    fwrite(&xxgd.nterms, 4, 1, filez[2]);
    fwrite(&xxgd.eff_pars[0], 4, 9, filez[2]);
    fwrite(&elgd.finest[0], 4, 5, filez[2]);
    fwrite(&elgd.swpars[0], 4, 3, filez[2]);
    fwrite(&elgd.h_norm, 4, 1, filez[2]);
    fwrite(&xxgd.esc_file_flags[0], 4, 10, filez[2]);
    fwrite(&elgd.bg_err, 4, 1, filez[2]);
    fwrite(&elgd.encal_err, 4, 1, filez[2]);
    fwrite(&elgd.effcal_err, 4, 1, filez[2]);
    memset(xxgd.rdata, 0, reclen);
    fwrite(xxgd.rdata, reclen-176, 1, filez[2]); /* zeroes to fill reclen */
    fflush(filez[2]);
    for (i = 0; i < 6; ++i) {
      /* write projections etc. to .esc file */
      fseek(filez[2], (xxgd.numchs + i)*reclen, SEEK_SET);
      fwrite(xxgd.bspec[i], 4, xxgd.numchs, filez[2]);
    }

  } else {
    /* deal with the gates, not changes to the .esc file */
    if (step < 0) {
      /* undo previous edits */
      /*  first check to see if we need to save the _current_ gate
	  for possible recall later */
      if (esclev_undo.pos_gate == esclev_undo.max_gate) {
	save_esclev_now(mode);
	esclev_undo.pos_gate--;
      }
      i = (esclev_undo.pos_gate--) % 20;
#ifdef GTK
      if (esclev_undo.pos_gate < esclev_undo.min_gate)      set_gate_button_sens(-1);
      if (esclev_undo.pos_gate >= esclev_undo.min_gate - 2) set_gate_button_sens(2);
#endif
    } else {
      /* redo previously undone edits */
      i = (++esclev_undo.pos_gate + 1) % 20;
#ifdef GTK
      if (esclev_undo.pos_gate <= esclev_undo.min_gate)     set_gate_button_sens(1);
      if (esclev_undo.pos_gate >= esclev_undo.max_gate - 1) set_gate_button_sens(-2);
#endif
    }
    memcpy(&xxgd.spec[0][0], &esclev_undo.gates[i].spec[0][0], 24*MAXCHS);
    memcpy(&xxgd.name_gat[0], &esclev_undo.gates[i].name_gat[0], 80);
    i = (i - 1) % 20;
    memcpy(&xxgd.old_spec[0][0], &esclev_undo.gates[i].spec[0][0], 24*MAXCHS);
    memcpy(&xxgd.old_name_gat[0], &esclev_undo.gates[i].name_gat[0], 80);
    hilite(-1);
  }

  calc_peaks();
  disp_dat(0);
  return 0;
} /* undo_esclev */
