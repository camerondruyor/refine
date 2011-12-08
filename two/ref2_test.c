#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "ref_grid_import.h"
#include "ref_grid_export.h"
#include "ref_test.h"

#include "ref_adj.h"
#include "ref_grid.h"
#include "ref_node.h"
#include "ref_metric.h"
#include "ref_cell.h"

#include "ref_edge.h"

#include "ref_face.h"
#include "ref_sort.h"

int main( int argc, char *argv[] )
{
  REF_GRID ref_grid;
  REF_FACE ref_edge;
  REF_FACE ref_face;

  if (argc<2) 
    {
      printf("usage: %s filename.ugrid\n",argv[0]);
      return 0;
    }

  printf("reading %s\n",argv[1]);
  TSS(ref_grid_import_ugrid( argv[1], &ref_grid ),"from ugrid");
  printf("complete.\n");

  printf("edges.\n");
  TSS(ref_edge_create( &ref_edge, ref_grid ),"from ugrid");

  printf("faces.\n");
  TSS(ref_face_create( &ref_face, ref_grid ),"from ugrid");

  printf("vtk.\n");
  TSS(ref_grid_export_vtk(ref_grid, "ref2.vtk"),"to vtk");

  printf("free.\n");
  TSS(ref_grid_free(ref_grid),"free");

  printf("done.\n");
  return 0;
}
