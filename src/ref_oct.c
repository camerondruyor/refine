
/* Copyright 2006, 2014, 2021 United States Government as represented
 * by the Administrator of the National Aeronautics and Space
 * Administration. No copyright is claimed in the United States under
 * Title 17, U.S. Code.  All Other Rights Reserved.
 *
 * The refine version 3 unstructured grid adaptation platform is
 * licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * https://www.apache.org/licenses/LICENSE-2.0.
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include "ref_oct.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "ref_malloc.h"

REF_FCN REF_STATUS ref_oct_create(REF_OCT *ref_oct_ptr) {
  REF_OCT ref_oct;

  ref_malloc(*ref_oct_ptr, 1, REF_OCT_STRUCT);
  ref_oct = (*ref_oct_ptr);

  ref_oct->bbox[0] = 0.0;
  ref_oct->bbox[1] = 1.0;
  ref_oct->bbox[2] = 0.0;
  ref_oct->bbox[3] = 1.0;
  ref_oct->bbox[4] = 0.0;
  ref_oct->bbox[5] = 1.0;

  ref_oct->n = 1;
  ref_oct->max = 1024;
  ref_oct->children = NULL;
  ref_malloc_init(ref_oct->children, 8 * ref_oct->max, REF_INT, REF_EMPTY);

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_oct_free(REF_OCT ref_oct) {
  if (NULL == (void *)ref_oct) return REF_NULL;
  ref_free(ref_oct->children);
  ref_free(ref_oct);
  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_oct_split(REF_OCT ref_oct, REF_INT node) {
  REF_INT i;
  for (i = 0; i < 8; i++) {
    REIS(REF_EMPTY, ref_oct->children[i + 8 * node], "child not empty");
  }
  if (ref_oct->n + 8 > ref_oct->max) {
    THROW("out of children");
  }
  for (i = 0; i < 8; i++) {
    ref_oct->children[i + 8 * node] = ref_oct->n;
    (ref_oct->n)++;
  }
  return REF_SUCCESS;
}

REF_FCN static REF_STATUS ref_oct_tec_node(REF_OCT ref_oct, REF_INT node,
                                           REF_DBL *bbox, FILE *f) {
  REF_DBL box[6];

  RAS(0 <= node && node < ref_oct->n, "out of range node");
  fprintf(f, "%f %f %f\n", bbox[0], bbox[2], bbox[4]);
  fprintf(f, "%f %f %f\n", bbox[1], bbox[2], bbox[4]);
  fprintf(f, "%f %f %f\n", bbox[1], bbox[3], bbox[4]);
  fprintf(f, "%f %f %f\n", bbox[0], bbox[3], bbox[4]);
  fprintf(f, "%f %f %f\n", bbox[0], bbox[2], bbox[5]);
  fprintf(f, "%f %f %f\n", bbox[1], bbox[2], bbox[5]);
  fprintf(f, "%f %f %f\n", bbox[1], bbox[3], bbox[5]);
  fprintf(f, "%f %f %f\n", bbox[0], bbox[3], bbox[5]);

  box[0] = bbox[0];
  box[1] = 0.5 * (bbox[0] + bbox[1]);
  box[2] = bbox[2];
  box[3] = 0.5 * (bbox[2] + bbox[3]);
  box[4] = bbox[4];
  box[5] = 0.5 * (bbox[4] + bbox[5]);
  if (REF_EMPTY != ref_oct->children[0 + 8 * node])
    RSS(ref_oct_tec_node(ref_oct, ref_oct->children[0 + 8 * node], box, f),
        "c 0");

  box[0] = 0.5 * (bbox[0] + bbox[1]);
  box[1] = bbox[1];
  box[2] = bbox[2];
  box[3] = 0.5 * (bbox[2] + bbox[3]);
  box[4] = bbox[4];
  box[5] = 0.5 * (bbox[4] + bbox[5]);
  if (REF_EMPTY != ref_oct->children[1 + 8 * node])
    RSS(ref_oct_tec_node(ref_oct, ref_oct->children[1 + 8 * node], box, f),
        "c 1");

  box[0] = 0.5 * (bbox[0] + bbox[1]);
  box[1] = bbox[1];
  box[2] = 0.5 * (bbox[2] + bbox[3]);
  box[3] = bbox[3];
  box[4] = bbox[4];
  box[5] = 0.5 * (bbox[4] + bbox[5]);
  if (REF_EMPTY != ref_oct->children[2 + 8 * node])
    RSS(ref_oct_tec_node(ref_oct, ref_oct->children[2 + 8 * node], box, f),
        "c 2");

  box[0] = bbox[0];
  box[1] = 0.5 * (bbox[0] + bbox[1]);
  box[2] = 0.5 * (bbox[2] + bbox[3]);
  box[3] = bbox[3];
  box[4] = bbox[4];
  box[5] = 0.5 * (bbox[4] + bbox[5]);
  if (REF_EMPTY != ref_oct->children[3 + 8 * node])
    RSS(ref_oct_tec_node(ref_oct, ref_oct->children[3 + 8 * node], box, f),
        "c 3");

  box[0] = bbox[0];
  box[1] = 0.5 * (bbox[0] + bbox[1]);
  box[2] = bbox[2];
  box[3] = 0.5 * (bbox[2] + bbox[3]);
  box[4] = 0.5 * (bbox[4] + bbox[5]);
  box[5] = bbox[5];
  if (REF_EMPTY != ref_oct->children[4 + 8 * node])
    RSS(ref_oct_tec_node(ref_oct, ref_oct->children[4 + 8 * node], box, f),
        "c 4");

  box[0] = 0.5 * (bbox[0] + bbox[1]);
  box[1] = bbox[1];
  box[2] = bbox[2];
  box[3] = 0.5 * (bbox[2] + bbox[3]);
  box[4] = 0.5 * (bbox[4] + bbox[5]);
  box[5] = bbox[5];
  if (REF_EMPTY != ref_oct->children[5 + 8 * node])
    RSS(ref_oct_tec_node(ref_oct, ref_oct->children[5 + 8 * node], box, f),
        "c 5");

  box[0] = 0.5 * (bbox[0] + bbox[1]);
  box[1] = bbox[1];
  box[2] = 0.5 * (bbox[2] + bbox[3]);
  box[3] = bbox[3];
  box[4] = 0.5 * (bbox[4] + bbox[5]);
  box[5] = bbox[5];
  if (REF_EMPTY != ref_oct->children[6 + 8 * node])
    RSS(ref_oct_tec_node(ref_oct, ref_oct->children[6 + 8 * node], box, f),
        "c 6");

  box[0] = bbox[0];
  box[1] = 0.5 * (bbox[0] + bbox[1]);
  box[2] = 0.5 * (bbox[2] + bbox[3]);
  box[3] = bbox[3];
  box[4] = 0.5 * (bbox[4] + bbox[5]);
  box[5] = bbox[5];
  if (REF_EMPTY != ref_oct->children[7 + 8 * node])
    RSS(ref_oct_tec_node(ref_oct, ref_oct->children[7 + 8 * node], box, f),
        "c 7");

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_oct_tec(REF_OCT ref_oct, const char *filename) {
  FILE *f;
  REF_INT i;
  const char *zonetype = "febrick";
  f = fopen(filename, "w");
  if (NULL == (void *)f) printf("unable to open %s\n", filename);
  RNS(f, "unable to open file");

  fprintf(f, "title=\"tecplot refine octree\"\n");
  fprintf(f, "variables = \"x\" \"y\" \"z\"\n");

  fprintf(f,
          "zone t=\"octree\", nodes=%d, elements=%d, datapacking=%s, "
          "zonetype=%s\n",
          8 * ref_oct->n, ref_oct->n, "point", zonetype);

  RSS(ref_oct_tec_node(ref_oct, 0, ref_oct->bbox, f), "draw root box");

  for (i = 0; i < ref_oct->n; i++) {
    fprintf(f, "%d %d %d %d %d %d %d %d\n", 1 + 8 * i, 2 + 8 * i, 3 + 8 * i,
            4 + 8 * i, 5 + 8 * i, 6 + 8 * i, 7 + 8 * i, 8 + 8 * i);
  }
  fclose(f);
  return REF_SUCCESS;
}