
/* Copyright 2014 United States Government as represented by the
 * Administrator of the National Aeronautics and Space
 * Administration. No copyright is claimed in the United States under
 * Title 17, U.S. Code.  All Other Rights Reserved.
 *
 * The refine platform is licensed under the Apache License, Version
 * 2.0 (the "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0.
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ref_defs.h"

#include "ref_args.h"
#include "ref_mpi.h"

#include "ref_geom.h"
#include "ref_grid.h"

#include "ref_histogram.h"
#include "ref_metric.h"
#include "ref_split.h"
#include "ref_validation.h"

#include "ref_export.h"
#include "ref_gather.h"
#include "ref_import.h"
#include "ref_part.h"

#include "ref_malloc.h"

static void usage(const char *name) {
  printf("usage: \n %s [--help] <command> [<args>]\n", name);
  printf("\n");
  printf("ref commands:\n");
  printf("  bootstrap   Create initial grid from EGADS file\n");
  printf("  fill        Fill a surface shell mesh with a volume.\n");
  printf("  location    Report the locations of verticies in the mesh.\n");
  printf("  multiscale  Extract mesh surface.\n");
  printf("  surface     Extract mesh surface.\n");
  printf("  translate   Convert mesh formats.\n");
}
static void bootstrap_help(const char *name) {
  printf("usage: \n %s boostrap project.egads [-t]\n", name);
  printf("  -t  tecplot movie of surface curvature adaptation\n");
  printf("        in files ref_gather_movie.tec and ref_gather_histo.tec\n");
  printf("\n");
}
static void fill_help(const char *name) {
  printf("usage: \n %s fill surface.meshb volume.meshb\n", name);
  printf("\n");
}
static void location_help(const char *name) {
  printf("usage: \n %s location input.meshb node_index node_index ...\n", name);
  printf("  node_index is zero-based\n");
  printf("\n");
}
static void multiscale_help(const char *name) {
  printf(
      "usage: \n %s multiscale input_mesh.extension scalar.solb metric.solb "
      "complexity\n",
      name);
  printf("\n");
}
static void surface_help(const char *name) {
  printf("usage: \n %s surface input_mesh.extension [surface_mesh.tec] \n",
         name);
  printf("\n");
}
static void translate_help(const char *name) {
  printf("usage: \n %s translate input_mesh.extension output_mesh.extension \n",
         name);
  printf("\n");
}

static REF_STATUS bootstrap(REF_MPI ref_mpi, int argc, char *argv[]) {
  size_t end_of_string;
  char project[1000];
  char filename[1024];
  REF_GRID ref_grid = NULL;
  REF_DBL params[3];
  REF_INT t_pos = REF_EMPTY;

  if (ref_mpi_para(ref_mpi)) {
    RSS(REF_IMPLEMENT, "ref bootstrap is not parallel");
  }
  if (argc < 3) goto shutdown;
  end_of_string = MIN(1023, strlen(argv[2]));
  if (7 > end_of_string ||
      strncmp(&(argv[2][end_of_string - 6]), ".egads", 6) != 0)
    goto shutdown;
  strncpy(project, argv[2], end_of_string - 6);
  project[end_of_string - 6] = '\0';

  RSS(ref_grid_create(&ref_grid, ref_mpi), "create");
  printf("loading %s.egads\n", project);
  RSS(ref_geom_egads_load(ref_grid_geom(ref_grid), argv[2]), "ld egads");
  ref_mpi_stopwatch_stop(ref_mpi, "egads load");

  printf("initial tessellation\n");
  RSS(ref_geom_egads_suggest_tess_params(ref_grid, params), "suggest params");
  RSS(ref_geom_egads_tess(ref_grid, params), "tess egads");
  ref_mpi_stopwatch_stop(ref_mpi, "egads tess");
  sprintf(filename, "%s-init.meshb", project);
  RSS(ref_export_by_extension(ref_grid, filename), "tess export");
  sprintf(filename, "%s-init-geom.tec", project);
  RSS(ref_geom_tec(ref_grid, filename), "geom export");
  sprintf(filename, "%s-init-surf.tec", project);
  RSS(ref_export_tec_surf(ref_grid, filename), "dbg surf");
  ref_mpi_stopwatch_stop(ref_mpi, "export init-surf");
  printf("verify topo\n");
  RSS(ref_geom_verify_topo(ref_grid), "adapt topo");
  printf("verify EGADS param\n");
  RSS(ref_geom_verify_param(ref_grid), "egads params");
  printf("constrain all\n");
  RSS(ref_geom_constrain_all(ref_grid), "constrain");
  printf("verify constrained param\n");
  RSS(ref_geom_verify_param(ref_grid), "constrained params");
  printf("verify manifold\n");
  RSS(ref_validation_boundary_manifold(ref_grid), "manifold");
  ref_mpi_stopwatch_stop(ref_mpi, "tess verification");

  RXS(ref_args_find(argc, argv, "-t", &t_pos), REF_NOT_FOUND, "arg search");
  if (REF_EMPTY != t_pos)
    RSS(ref_gather_tec_movie_record_button(ref_grid_gather(ref_grid), REF_TRUE),
        "movie on");

  RSS(ref_adapt_surf_to_geom(ref_grid), "ad");
  RSS(ref_geom_report_tri_area_normdev(ref_grid), "tri status");
  printf("verify topo\n");
  RSS(ref_geom_verify_topo(ref_grid), "adapt topo");
  printf("verify param\n");
  RSS(ref_geom_verify_param(ref_grid), "adapt params");
  ref_mpi_stopwatch_stop(ref_mpi, "surf verification");
  sprintf(filename, "%s-adapt-geom.tec", project);
  RSS(ref_geom_tec(ref_grid, filename), "geom export");
  sprintf(filename, "%s-adapt-surf.tec", project);
  RSS(ref_export_tec_surf(ref_grid, filename), "dbg surf");
  sprintf(filename, "%s-adapt-surf.meshb", project);
  printf("export %s\n", filename);
  RSS(ref_export_by_extension(ref_grid, filename), "surf export");
  ref_mpi_stopwatch_stop(ref_mpi, "export adapt surf");

  RSS(ref_geom_tetgen_volume(ref_grid), "tetgen surface to volume ");
  ref_mpi_stopwatch_stop(ref_mpi, "fill volume");

  RSS(ref_split_edge_geometry(ref_grid), "split geom");
  ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "split geom");

  sprintf(filename, "%s-vol.meshb", project);
  printf("export %s\n", filename);
  RSS(ref_export_by_extension(ref_grid, filename), "vol export");
  ref_mpi_stopwatch_stop(ref_mpi, "export volume");

  RSS(ref_metric_interpolated_curvature(ref_grid), "interp curve");
  ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "curvature");
  RSS(ref_histogram_quality(ref_grid), "gram");
  RSS(ref_histogram_ratio(ref_grid), "gram");
  ref_mpi_stopwatch_stop(ref_grid_mpi(ref_grid), "histogram");

  RSS(ref_grid_free(ref_grid), "free grid");

  return REF_SUCCESS;
shutdown:
  if (ref_mpi_para(ref_mpi)) bootstrap_help(argv[0]);
  return REF_FAILURE;
}

static REF_STATUS fill(REF_MPI ref_mpi, int argc, char *argv[]) {
  char *out_file;
  char *in_file;
  REF_GRID ref_grid = NULL;

  if (ref_mpi_para(ref_mpi)) {
    RSS(REF_IMPLEMENT, "ref fill is not parallel");
  }
  if (argc < 4) goto shutdown;
  in_file = argv[2];
  out_file = argv[3];

  printf("import %s\n", in_file);
  RSS(ref_import_by_extension(&ref_grid, ref_mpi, in_file), "load surface");

  RSS(ref_geom_tetgen_volume(ref_grid), "tetgen surface to volume ");

  printf("export %s\n", out_file);
  RSS(ref_export_by_extension(ref_grid, out_file), "vol export");

  RSS(ref_grid_free(ref_grid), "create");

  return REF_SUCCESS;
shutdown:
  if (ref_mpi_para(ref_mpi)) fill_help(argv[0]);
  return REF_FAILURE;
}

static REF_STATUS location(REF_MPI ref_mpi, int argc, char *argv[]) {
  char *in_file;
  REF_INT pos, global, local;
  REF_GRID ref_grid = NULL;

  if (ref_mpi_para(ref_mpi)) {
    RSS(REF_IMPLEMENT, "ref location is not parallel");
  }
  if (argc < 4) goto shutdown;
  in_file = argv[2];

  printf("import %s\n", in_file);
  RSS(ref_import_by_extension(&ref_grid, ref_mpi, in_file), "load surface");

  for (pos = 3; pos < argc; pos++) {
    global = atoi(argv[pos]);
    printf("global index %d\n", global);
    RSS(ref_node_local(ref_grid_node(ref_grid), global, &local),
        "global node_index not found");
    RSS(ref_node_location(ref_grid_node(ref_grid), local), "location");
  }

  RSS(ref_grid_free(ref_grid), "create");

  return REF_SUCCESS;
shutdown:
  if (ref_mpi_para(ref_mpi)) location_help(argv[0]);
  return REF_FAILURE;
}

static REF_STATUS surface(REF_MPI ref_mpi, int argc, char *argv[]) {
  char *out_file;
  char *in_file;
  char filename[1024];
  REF_GRID ref_grid = NULL;

  if (argc < 3) goto shutdown;
  in_file = argv[2];
  if (argc < 4) {
    RAS(strlen(in_file) < 1014, "input filename too long (>1014)");
    sprintf(filename, "%s-surf.tec", in_file);
    out_file = filename;
  } else {
    out_file = argv[3];
  }

  ref_mpi_stopwatch_start(ref_mpi);

  if (ref_mpi_para(ref_mpi)) {
    if (ref_mpi_once(ref_mpi)) printf("part %s\n", in_file);
    RSS(ref_part_by_extension(&ref_grid, ref_mpi, in_file), "part");
    ref_mpi_stopwatch_stop(ref_mpi, "part");
  } else {
    if (ref_mpi_once(ref_mpi)) printf("import %s\n", in_file);
    RSS(ref_import_by_extension(&ref_grid, ref_mpi, in_file), "import");
    ref_mpi_stopwatch_stop(ref_mpi, "import");
  }

  if (ref_mpi_para(ref_mpi)) {
    if (ref_mpi_once(ref_mpi)) printf("gather %s\n", out_file);
    RSS(ref_gather_scalar_surf_tec(ref_grid, 0, NULL, NULL, out_file),
        "gather surf tec");
    ref_mpi_stopwatch_stop(ref_mpi, "gather");
  } else {
    if (ref_mpi_once(ref_mpi)) printf("export %s\n", out_file);
    RSS(ref_export_tec_surf(ref_grid, out_file), "export tec surf");
    ref_mpi_stopwatch_stop(ref_mpi, "export");
  }

  RSS(ref_grid_free(ref_grid), "free grid");

  return REF_SUCCESS;
shutdown:
  if (ref_mpi_para(ref_mpi)) surface_help(argv[0]);
  return REF_FAILURE;
}

static REF_STATUS multiscale(REF_MPI ref_mpi, int argc, char *argv[]) {
  char *out_metric;
  char *in_mesh;
  char *in_scalar;
  REF_GRID ref_grid = NULL;
  REF_INT ldim;
  REF_DBL *scalar, *metric;
  REF_INT p;
  REF_DBL gradation, complexity, current_complexity;
  REF_RECON_RECONSTRUCTION reconstruction = REF_RECON_L2PROJECTION;
  REF_INT pos;

  if (argc < 6) goto shutdown;
  in_mesh = argv[2];
  in_scalar = argv[3];
  out_metric = argv[4];
  complexity = atof(argv[5]);

  p = 2;
  RXS(ref_args_find(argc, argv, "-p", &pos), REF_NOT_FOUND, "arg search");
  if (REF_EMPTY != pos) {
    if (pos >= argc - 1) {
      printf("option missing value: -p <norm power>\n");
      goto shutdown;
    }
    p = atoi(argv[pos + 1]);
  }

  gradation = -1.0;
  RXS(ref_args_find(argc, argv, "-g", &pos), REF_NOT_FOUND, "arg search");
  if (REF_EMPTY != pos) {
    if (pos >= argc - 1) {
      printf("option missing value: -g <gradation>\n");
      goto shutdown;
    }
    gradation = atof(argv[pos + 1]);
  }

  if (ref_mpi_once(ref_mpi)) {
    printf("complexity %f\n", complexity);
    printf("Lp=%d\n", p);
    printf("gradation %f\n", gradation);
    printf("reconstruction %d\n", (int)reconstruction);
  }

  ref_mpi_stopwatch_start(ref_mpi);

  if (ref_mpi_para(ref_mpi)) {
    if (ref_mpi_once(ref_mpi)) printf("part %s\n", in_mesh);
    RSS(ref_part_by_extension(&ref_grid, ref_mpi, in_mesh), "part");
    ref_mpi_stopwatch_stop(ref_mpi, "part");
  } else {
    if (ref_mpi_once(ref_mpi)) printf("import %s\n", in_mesh);
    RSS(ref_import_by_extension(&ref_grid, ref_mpi, in_mesh), "import");
    ref_mpi_stopwatch_stop(ref_mpi, "import");
  }

  if (ref_mpi_once(ref_mpi)) printf("part scalar %s\n", in_scalar);
  RSS(ref_part_scalar(ref_grid_node(ref_grid), &ldim, &scalar, in_scalar),
      "part scalar");
  REIS(1, ldim, "expected one scalar");
  ref_mpi_stopwatch_stop(ref_mpi, "part scalar");

  ref_malloc(metric, 6 * ref_node_max(ref_grid_node(ref_grid)), REF_DBL);
  RSS(ref_metric_lp(metric, ref_grid, scalar, NULL, reconstruction, p,
                    gradation, complexity),
      "lp norm");
  ref_mpi_stopwatch_stop(ref_mpi, "compute metric");

  RSS(ref_metric_complexity(metric, ref_grid, &current_complexity), "cmp");
  if (ref_mpi_once(ref_mpi))
    printf("actual complexity %e\n", current_complexity);
  RSS(ref_metric_to_node(metric, ref_grid_node(ref_grid)), "set node");

  ref_free(metric);
  ref_free(scalar);

  if (ref_mpi_once(ref_mpi)) printf("gather %s\n", out_metric);
  RSS(ref_gather_metric(ref_grid, out_metric), "gather metric");
  ref_mpi_stopwatch_stop(ref_mpi, "gather metric");

  RSS(ref_grid_free(ref_grid), "free grid");

  return REF_SUCCESS;
shutdown:
  if (ref_mpi_para(ref_mpi)) multiscale_help(argv[0]);
  return REF_FAILURE;
}

static REF_STATUS translate(REF_MPI ref_mpi, int argc, char *argv[]) {
  char *out_file;
  char *in_file;
  REF_GRID ref_grid = NULL;

  if (argc < 4) goto shutdown;
  in_file = argv[2];
  out_file = argv[3];

  ref_mpi_stopwatch_start(ref_mpi);

  if (ref_mpi_para(ref_mpi)) {
    if (ref_mpi_once(ref_mpi)) printf("part %s\n", in_file);
    RSS(ref_part_by_extension(&ref_grid, ref_mpi, in_file), "part");
    ref_mpi_stopwatch_stop(ref_mpi, "part");
  } else {
    if (ref_mpi_once(ref_mpi)) printf("import %s\n", in_file);
    RSS(ref_import_by_extension(&ref_grid, ref_mpi, in_file), "import");
    ref_mpi_stopwatch_stop(ref_mpi, "import");
  }

  if (ref_mpi_para(ref_mpi)) {
    if (ref_mpi_once(ref_mpi)) printf("gather %s\n", out_file);
    RSS(ref_gather_by_extension(ref_grid, out_file), "gather");
    ref_mpi_stopwatch_stop(ref_mpi, "gather");
  } else {
    if (ref_mpi_once(ref_mpi)) printf("export %s\n", out_file);
    RSS(ref_export_by_extension(ref_grid, out_file), "export");
    ref_mpi_stopwatch_stop(ref_mpi, "export");
  }

  RSS(ref_grid_free(ref_grid), "free grid");

  return REF_SUCCESS;
shutdown:
  if (ref_mpi_para(ref_mpi)) translate_help(argv[0]);
  return REF_FAILURE;
}

int main(int argc, char *argv[]) {
  REF_MPI ref_mpi;
  REF_INT help_pos = REF_EMPTY;

  RSS(ref_mpi_start(argc, argv), "start");
  RSS(ref_mpi_create(&ref_mpi), "make mpi");
  ref_mpi_stopwatch_start(ref_mpi);

  RXS(ref_args_find(argc, argv, "--help", &help_pos), REF_NOT_FOUND,
      "arg search");
  if (REF_EMPTY == help_pos) {
    RXS(ref_args_find(argc, argv, "-h", &help_pos), REF_NOT_FOUND,
        "arg search");
  }

  if (1 == argc || 1 == help_pos) {
    if (ref_mpi_once(ref_mpi)) usage(argv[0]);
    goto shutdown;
  }

  if (strncmp(argv[1], "b", 1) == 0) {
    if (REF_EMPTY == help_pos) {
      RSS(bootstrap(ref_mpi, argc, argv), "bootstrap");
    } else {
      if (ref_mpi_once(ref_mpi)) bootstrap_help(argv[0]);
      goto shutdown;
    }
  } else if (strncmp(argv[1], "f", 1) == 0) {
    if (REF_EMPTY == help_pos) {
      RSS(fill(ref_mpi, argc, argv), "fill");
    } else {
      if (ref_mpi_once(ref_mpi)) fill_help(argv[0]);
      goto shutdown;
    }
  } else if (strncmp(argv[1], "l", 1) == 0) {
    if (REF_EMPTY == help_pos) {
      RSS(location(ref_mpi, argc, argv), "location");
    } else {
      if (ref_mpi_once(ref_mpi)) location_help(argv[0]);
      goto shutdown;
    }
  } else if (strncmp(argv[1], "m", 1) == 0) {
    if (REF_EMPTY == help_pos) {
      RSS(multiscale(ref_mpi, argc, argv), "multiscale");
    } else {
      if (ref_mpi_once(ref_mpi)) multiscale_help(argv[0]);
      goto shutdown;
    }
  } else if (strncmp(argv[1], "s", 1) == 0) {
    if (REF_EMPTY == help_pos) {
      RSS(surface(ref_mpi, argc, argv), "surface");
    } else {
      if (ref_mpi_once(ref_mpi)) surface_help(argv[0]);
      goto shutdown;
    }
  } else if (strncmp(argv[1], "t", 1) == 0) {
    if (REF_EMPTY == help_pos) {
      RSS(translate(ref_mpi, argc, argv), "translate");
    } else {
      if (ref_mpi_once(ref_mpi)) translate_help(argv[0]);
      goto shutdown;
    }
  } else {
    if (ref_mpi_para(ref_mpi)) usage(argv[0]);
    goto shutdown;
  }

  ref_mpi_stopwatch_stop(ref_mpi, "done.");
shutdown:
  RSS(ref_mpi_free(ref_mpi), "mpi free");
  RSS(ref_mpi_stop(), "stop");

  return 0;
}
