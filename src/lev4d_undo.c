#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "levit8r.h"
#include "util.h"

typedef struct {
  float eff_sp[MAXCHS], energy_sp[MAXCHS];
  float elo_sp[MAXCHS], ehi_sp[MAXCHS], ewid_sp[MAXCHS];
  int   luch[MAXCHS+1];
  short looktab[16384];
  int   nclook, lookmin, lookmax;
  float  bg_err, encal_err, effcal_err;
  float  finest[5], swpars[3], h_norm, t_norm, q_norm;
  float  bspec[6][MAXCHS];
} undo_lev_data;

typedef struct {
  float  spec[6][MAXCHS];
  char   name_gat[80];
} undo_gate_data;

struct {
  undo_lev_data  esc[10];
  undo_gate_data gates[20];
  int  max_lev, max_gate;
  int  pos_lev, pos_gate;
  int  min_lev, min_gate;
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
    esclev_undo.max_lev = esclev_undo.max_gate = -1;
    esclev_undo.pos_lev = esclev_undo.pos_gate = -1;
    esclev_undo.min_lev = 0;
    esclev_undo.min_gate = 2;
    esclev_undo_ready = 1;
  }

  if (mode < 0) {
    esclev_undo.max_lev = esclev_undo.max_gate = -1;
    esclev_undo.pos_lev = esclev_undo.pos_gate = -1;
    esclev_undo.min_lev = 0;
    esclev_undo.min_gate = 1;
    return 0;
  }

  if (mode < 4) {
    if (xxgd.le2pro2d) {
      tell("Cannot save/undo enhanced background!\n"
	   "Resetting undo info.\n");
      save_esclev_now(-1);
      return 1;
    }
    i = (++esclev_undo.pos_lev) % 10;
    memcpy(&esclev_undo.esc[i].eff_sp[0], &xxgd.eff_sp[0], 4*(6*MAXCHS + 1));
    memcpy(&esclev_undo.esc[i].looktab[0], &xxgd.looktab[0], 2*16384 + 4*3);
    memcpy(&esclev_undo.esc[i].bg_err,  &elgd.bg_err,  14*4);
    memcpy(&esclev_undo.esc[i].bspec[0][0], &xxgd.bspec[0][0], 24*MAXCHS);
    esclev_undo.max_lev = esclev_undo.pos_lev;
    if (esclev_undo.min_lev < esclev_undo.pos_lev - 9)
      esclev_undo.min_lev = esclev_undo.pos_lev - 9;
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
  int  i;
  char dattim[20];

  if (!esclev_undo_ready || step == 0) return 1;

  /* first check to see that we have gates/changes to undo or redo */
  if (step < 0 && 
      ((mode < 4  && esclev_undo.pos_lev < esclev_undo.min_lev) ||
       (mode >= 4 && esclev_undo.pos_gate < esclev_undo.min_gate))) {
    tell("%cNo more undo information...\n", (char) 7);
    return 1;
  }
  if (step > 0 &&
      ((mode < 4  && esclev_undo.pos_lev >= esclev_undo.max_lev - 1) ||
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
      if (esclev_undo.pos_lev == esclev_undo.max_lev) {
	if (save_esclev_now(mode)) return 1;
	esclev_undo.pos_lev--;
      }
      i = (esclev_undo.pos_lev--) % 10;
    } else {
      /* redo previously undone edits */
      i = (++esclev_undo.pos_lev + 1) % 10;
    }
    memcpy(&xxgd.eff_sp[0], &esclev_undo.esc[i].eff_sp[0], 4*(6*MAXCHS + 1));
    memcpy(&xxgd.looktab[0], &esclev_undo.esc[i].looktab[0], 2*16384 + 4*3);
    memcpy(&elgd.bg_err,  &esclev_undo.esc[i].bg_err,  14*4);
    memcpy(&xxgd.bspec[0][0], &esclev_undo.esc[i].bspec[0][0], 24*MAXCHS);

   /* log the undo/redo to the .lev_log file */
    datetime(dattim);
    if (step < 0) {
      fprintf(filez[26], "%s: Undo!\n", dattim);
    } else {
      fprintf(filez[26], "%s: Redo!\n", dattim);
    }
    fflush(filez[26]);
    /* update the .lev file */
    read_write_l4d_file("WRITE");

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
