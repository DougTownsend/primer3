/* DPT-dump instrumentation for thal.c.

   Enable by setting either env var before running:
     THAL_DPT_DUMP_MD=<path|->     markdown snapshots (- = stderr)
     THAL_DPT_DUMP_JSON=<path|->   JSON stream for matplotlib renderer

   When neither is set, every entry point here is a cheap no-op.
   See docs/make_hairpin_figures.py for the JSON consumer. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "thal_internal.h"

static FILE *dpt_dump_md_fp   = NULL;
static FILE *dpt_dump_json_fp = NULL;
static int   dpt_dump_inited  = 0;
static int   dpt_dump_step    = 0;
static int   dpt_dump_json_first = 1;

static void dpt_dump_init(void) {
   if (dpt_dump_inited) return;
   dpt_dump_inited = 1;
   const char *md_path = getenv("THAL_DPT_DUMP_MD");
   if (md_path) {
      dpt_dump_md_fp = (strcmp(md_path, "-") == 0) ? stderr : fopen(md_path, "w");
   }
   const char *json_path = getenv("THAL_DPT_DUMP_JSON");
   if (json_path) {
      dpt_dump_json_fp = (strcmp(json_path, "-") == 0) ? stderr : fopen(json_path, "w");
      if (dpt_dump_json_fp) fprintf(dpt_dump_json_fp, "[\n");
   }
}

void dpt_dump_close(void) {
   if (dpt_dump_md_fp && dpt_dump_md_fp != stderr) {
      fclose(dpt_dump_md_fp);
   }
   dpt_dump_md_fp = NULL;
   if (dpt_dump_json_fp) {
      if (dpt_dump_json_fp != stderr) {
         fprintf(dpt_dump_json_fp, "\n]\n");
         fclose(dpt_dump_json_fp);
      }
   }
   dpt_dump_json_fp = NULL;
   dpt_dump_inited = 0;
   dpt_dump_step = 0;
   dpt_dump_json_first = 1;
}

/* Print one (i,j) cell to the JSON stream. Helper for dpt_dump.
   The "init" state (valid bp, h==0, no predecessor) is emitted as
   `state="init"` so the renderer can render it as gray rather than as
   a +1M-cal/mol outlier. */
static void dpt_cell_json(FILE *f, int i, int j, double h, double s,
                          int tb_i, int tb_j, double T) {
   fprintf(f, "{\"i\":%d,\"j\":%d", i, j);
   if (tb_i == -2) {
      fprintf(f, ",\"state\":\"unvisited\",\"h\":null,\"s\":null,\"g\":null");
   } else if (!isFinite(h)) {
      fprintf(f, ",\"state\":\"invalid\",\"h\":null,\"s\":%g,\"g\":null", s);
   } else if (h == 0.0 && s == MinEntropy) {
      fprintf(f, ",\"state\":\"init\",\"h\":0,\"s\":%g,\"g\":null", s);
   } else {
      double g = h - T * s;
      fprintf(f, ",\"state\":\"filled\",\"h\":%g,\"s\":%g,\"g\":%g", h, s, g);
   }
   fprintf(f, ",\"tb_i\":%d,\"tb_j\":%d}", tb_i, tb_j);
}

/* dpt_dump: snapshot the DPT after some step. `is_hairpin` flips index
   semantics for visualization (hairpin: j > i only). */
void dpt_dump(struct dpt_entry **dpt, int len1, int len2,
              int is_hairpin, const char *label) {
   if (!dpt_dump_inited) dpt_dump_init();
   if (!dpt_dump_md_fp && !dpt_dump_json_fp) return;
   int step = ++dpt_dump_step;
   double T = TEMP_KELVIN;

   if (dpt_dump_md_fp) {
      FILE *f = dpt_dump_md_fp;
      fprintf(f, "\n### Step %d: %s\n\n", step, label);
      fprintf(f, "| i \\\\ j |");
      for (int j = 1; j <= len2; ++j) fprintf(f, " **%d** |", j);
      fprintf(f, "\n|---|");
      for (int j = 1; j <= len2; ++j) fprintf(f, "---|");
      fprintf(f, "\n");
      for (int i = 1; i <= len1; ++i) {
         fprintf(f, "| **%d** |", i);
         for (int j = 1; j <= len2; ++j) {
            int valid_cell = is_hairpin ? (j > i) : 1;
            if (!valid_cell) {
               fprintf(f, " · |");
               continue;
            }
            if (dpt[i][j].tb_i == -2) {
               /* unvisited */
               fprintf(f, "   |");
            } else if (!isFinite(dpt[i][j].h)) {
               fprintf(f, " ∞ |");
            } else if (dpt[i][j].h == 0.0 && dpt[i][j].s == MinEntropy) {
               /* hairpin init state (valid bp, no DP value yet) */
               fprintf(f, " init |");
            } else {
               double g = dpt[i][j].h - T * dpt[i][j].s;
               if (dpt[i][j].tb_i >= 0)
                  fprintf(f, " %.0f<br>tb=%d,%d |", g, dpt[i][j].tb_i, dpt[i][j].tb_j);
               else
                  fprintf(f, " %.0f |", g);
            }
         }
         fprintf(f, "\n");
      }
      fprintf(f, "\n");
   }

   if (dpt_dump_json_fp) {
      FILE *f = dpt_dump_json_fp;
      if (!dpt_dump_json_first) fprintf(f, ",\n");
      dpt_dump_json_first = 0;
      fprintf(f, "  {\"step\":%d,\"label\":\"%s\",\"is_hairpin\":%s,\"len1\":%d,\"len2\":%d,\"temp\":%g,\"cells\":[\n",
              step, label, is_hairpin ? "true" : "false", len1, len2, T);
      int first = 1;
      for (int i = 1; i <= len1; ++i) {
         for (int j = 1; j <= len2; ++j) {
            int valid = is_hairpin ? (j > i) : 1;
            if (!valid) continue;
            if (!first) fprintf(f, ",\n");
            first = 0;
            fprintf(f, "    ");
            dpt_cell_json(f, i, j, dpt[i][j].h, dpt[i][j].s,
                          dpt[i][j].tb_i, dpt[i][j].tb_j, T);
         }
      }
      fprintf(f, "\n  ]}");
   }
}

/* Dump send5/hend5 — markdown only (matplotlib renderer doesn't need it). */
void send5_dump(double *send5, double *hend5, int len, const char *label) {
   if (!dpt_dump_inited) dpt_dump_init();
   if (!dpt_dump_md_fp) return;
   FILE *f = dpt_dump_md_fp;
   int step = ++dpt_dump_step;
   double T = TEMP_KELVIN;
   fprintf(f, "\n### Step %d: %s — send5 / hend5 prefix DP\n\n", step, label);
   fprintf(f, "| i |");
   for (int i = 0; i <= len; ++i) fprintf(f, " %d |", i);
   fprintf(f, "\n|---|");
   for (int i = 0; i <= len; ++i) fprintf(f, "---|");
   fprintf(f, "\n| dG |");
   for (int i = 0; i <= len; ++i) {
      if (isFinite(hend5[i]) && hend5[i] <= 0.0) {
         double g = hend5[i] - T * send5[i];
         fprintf(f, " %.0f |", g);
      } else if (isFinite(hend5[i])) {
         fprintf(f, " 0 |");
      } else {
         fprintf(f, " ∞ |");
      }
   }
   fprintf(f, "\n\n");
}
