
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

#include <stdio.h>
#include <stdlib.h>

#include "ref_phys.h"

#include "ref_args.h"
#include "ref_mpi.h"

#include "ref_grid.h"

#include "ref_gather.h"
#include "ref_malloc.h"
#include "ref_part.h"

int main(int argc, char *argv[]) {
  REF_INT laminar_flux_pos = REF_EMPTY;

  REF_MPI ref_mpi;
  RSS(ref_mpi_start(argc, argv), "start");
  RSS(ref_mpi_create(&ref_mpi), "create");

  RXS(ref_args_find(argc, argv, "--laminar-flux", &laminar_flux_pos),
      REF_NOT_FOUND, "arg search");

  if (laminar_flux_pos != REF_EMPTY) {
    REF_GRID ref_grid;
    REF_DBL mach, re, temperature;
    REF_DBL *primitive_dual, *dual_flux;
    REF_INT ldim;

    REIS(1, laminar_flux_pos,
         "required args: --laminar-flux grid.meshb primitive_dual.solb Mach Re "
         "T_K"
         "dual_flux.solb");
    if (8 > argc) {
      printf(
          "required args: --laminar-flux grid.meshb primitive_dual.solb Mach "
          "Re T_K"
          "dual_flux.solb\n");
      return REF_FAILURE;
    }
    mach = atof(argv[4]);
    re = atof(argv[5]);
    temperature = atof(argv[6]);
    printf("Reference Mach %f Re %e temperature %f\n", mach, re, temperature);

    printf("reading grid %s\n", argv[2]);
    RSS(ref_part_by_extension(&ref_grid, ref_mpi, argv[2]),
        "unable to load target grid in position 2");

    printf("reading primitive_dual %s\n", argv[3]);
    RSS(ref_part_scalar(ref_grid_node(ref_grid), &ldim, &primitive_dual,
                        argv[3]),
        "unable to load primitive_dual in position 3");
    REIS(10, ldim, "expected 10 (rho,u,v,w,p,5*adj) primitive_dual");
    ref_malloc(dual_flux, 20 * ref_node_max(ref_grid_node(ref_grid)), REF_DBL);

    printf("writing dual_flux %s\n", argv[5]);
    RSS(ref_gather_scalar(ref_grid, 20, dual_flux, argv[5]),
        "export dual_flux");

    RSS(ref_grid_free(ref_grid), "free");
    RSS(ref_mpi_free(ref_mpi), "free");
    RSS(ref_mpi_stop(), "stop");
    return 0;
  }

  { /* x-Euler flux */
    REF_DBL state[5], direction[3];
    REF_DBL flux[5];
    state[0] = 1.0;
    state[1] = 0.2;
    state[2] = 0.0;
    state[3] = 0.0;
    state[4] = 1.0 / 1.4;
    direction[0] = 1.0;
    direction[1] = 0.0;
    direction[2] = 0.0;
    RSS(ref_phys_euler(state, direction, flux), "euler");
    RWDS(0.2, flux[0], -1, "mass flux");
    RWDS(0.04 + 1.0 / 1.4, flux[1], -1, "x mo flux");
    RWDS(0.0, flux[2], -1, "y mo flux");
    RWDS(0.0, flux[3], -1, "z mo flux");
    RWDS(0.504, flux[4], -1, "energy flux");
  }

  { /* Couette laminar flux */
    REF_DBL state[5], gradient[15], direction[3];
    REF_DBL flux[5];
    REF_DBL mach = 0.1, re = 10.0, temp = 273.0;
    REF_DBL dudy = 1.0, mu = 1.0;
    REF_DBL thermal_conductivity = mu / ((1.4 - 1.0) * 0.72);
    REF_DBL dpdx = 1.0 / 1.4, dtdx = 1.0;
    REF_INT i;
    for (i = 0; i < 15; i++) gradient[i] = 0.0;
    gradient[1 + 3 * 1] = dudy;
    gradient[0 + 3 * 4] = dpdx;

    state[0] = 1.0;
    state[1] = 0.1;
    state[2] = 0.0;
    state[3] = 0.0;
    state[4] = 1.0 / 1.4;
    direction[0] = 1.0;
    direction[1] = 0.0;
    direction[2] = 0.0;
    RSS(ref_phys_laminar(state, gradient, mach, re, temp, direction, flux),
        "euler");
    RWDS(0.0, flux[0], -1, "mass flux");
    RWDS(0.0, flux[1], -1, "x mo flux");
    RWDS(mach / re * mu * dudy * direction[0], flux[2], -1, "y mo flux");
    RWDS(0.0, flux[3], -1, "z mo flux");
    RWDS(
        mach / re *
            (mu * dudy * direction[0] * state[2] + thermal_conductivity * dtdx),
        flux[4], -1, "energy flux");
  }

  { /* bulk visc laminar flux */
    REF_DBL state[5], gradient[15], direction[3];
    REF_DBL flux[5];
    REF_DBL mach = 0.1, re = 10.0, temp = 273.0;
    REF_DBL dvdy = 1.0, mu = 1.0;
    REF_DBL thermal_conductivity = mu / ((1.4 - 1.0) * 0.72), dtdy = 1.0,
            dpdy = 1.0 / 1.4;
    REF_INT i;
    for (i = 0; i < 15; i++) gradient[i] = 0.0;
    gradient[1 + 3 * 2] = dvdy;
    gradient[1 + 3 * 4] = dpdy;

    state[0] = 1.0;
    state[1] = 0.1;
    state[2] = 0.0;
    state[3] = 0.0;
    state[4] = 1.0 / 1.4;
    direction[0] = 0.0;
    direction[1] = 1.0;
    direction[2] = 0.0;
    RSS(ref_phys_laminar(state, gradient, mach, re, temp, direction, flux),
        "euler");
    RWDS(0.0, flux[0], -1, "mass flux");
    RWDS(0.0, flux[1], -1, "x mo flux");
    RWDS(mach / re * mu * (4.0 / 3.0) * dvdy * direction[1], flux[2], -1,
         "y mo flux");
    RWDS(0.0, flux[3], -1, "z mo flux");
    RWDS(mach / re *
             (mu * (4.0 / 3.0) * dvdy * state[2] + thermal_conductivity * dtdy),
         flux[4], -1, "energy flux");
  }

  return 0;
}
