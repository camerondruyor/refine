
/* Michael A. Park
 * Computational Modeling & Simulation Branch
 * NASA Langley Research Center
 * Phone:(757)864-6604
 * Email:m.a.park@larc.nasa.gov 
 */
  
/* $Id$ */

#include <stdlib.h>
#include <stdio.h>
#include "gridfortran.h"
#include "grid.h"
#include "gridmetric.h"

static Grid *grid;

int gridcreate_( int *nnode, double *x, double *y, double *z ,
		 int *ncell, int *maxcell, int *c2n )
{
  int node, cell;
  int nodes[4];
  double xyz[3];
  grid = gridCreate( *nnode, *ncell, 5000, 0);
  for ( node=0; node<*nnode; node++) gridAddNode(grid,x[node],y[node],z[node]);
  printf("populated grid object with %d nodes\n",gridNNode(grid));
  for ( cell=0; cell<*ncell; cell++) {
    gridAddCell( grid,
		 c2n[cell+0*(*maxcell)] - 1,
		 c2n[cell+1*(*maxcell)] - 1,
		 c2n[cell+2*(*maxcell)] - 1,
		 c2n[cell+3*(*maxcell)] - 1 );
    gridCell(grid,cell,nodes);
    printf(" %8d AR %25.15e\n",cell,gridAR(grid,nodes));
    for(node=0;node<4;node++){
      gridNodeXYZ(grid,nodes[node],xyz);
      printf(" %2d AR %23.15e%23.15e%23.15e\n",
	     node,xyz[0],xyz[1],xyz[2]);
    }
  }
  printf("populated grid object with %d cells\n",gridNCell(grid));
  printf(" min AR %25.15e\n",gridMinAR(grid));
}


