
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

static Grid *grid;

int gridcreate_( int *maxnode, int *maxcell, int *maxface, int *maxedge )
{
  grid = gridCreate( *maxnode, *maxcell, *maxface, *maxedge);
  printf("%s:%d: created grid obj with %d nodes %d cells\n",
	 __FILE__, __LINE__,gridMaxNode(grid),gridMaxCell(grid));
}

int gridcreate( int *maxnode, int *maxcell, int *maxface, int *maxedge )
{ return gridcreate_(maxnode, maxcell, maxface, maxedge); }

int gridcreate__( int *maxnode, int *maxcell, int *maxface, int *maxedge )
{ return gridcreate_(maxnode, maxcell, maxface, maxedge); }

int GRIDCREATE( int *maxnode, int *maxcell, int *maxface, int *maxedge )
{ return gridcreate_(maxnode, maxcell, maxface, maxedge); }


