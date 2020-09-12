
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

#include "ref_iso.h"

#include <stdio.h>
#include <stdlib.h>

#include "ref_edge.h"
#include "ref_malloc.h"

REF_STATUS ref_iso_insert(REF_GRID *iso_grid_ptr, REF_GRID ref_grid,
                          REF_DBL *field) {
  REF_EDGE ref_edge;
  REF_INT *mark;

  RSS(ref_grid_create(iso_grid_ptr, ref_grid_mpi(ref_grid)), "create");
  RSS(ref_edge_create(&ref_edge, ref_grid), "create edge");
  ref_malloc_init(mark, ref_edge_n(ref_edge), REF_INT, 0);

  ref_free(mark);
  ref_edge_free(ref_edge);
  SUPRESS_UNUSED_COMPILER_WARNING(field);
  return REF_SUCCESS;
}
