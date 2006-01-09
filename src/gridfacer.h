
/* Michael A. Park
 * Computational AeroSciences Branch
 * NASA Langley Research Center
 * Phone:(757)864-6604
 * Email:Mike.Park@NASA.Gov 
 */
  
/* $Id$ */

#ifndef GRIDFACER_H
#define GRIDFACER_H

#include "refine_defs.h"
#include "grid.h"

BEGIN_C_DECLORATION

typedef struct GridFacer GridFacer;
struct GridFacer {
  Grid *grid;
  void *gridRubyVALUEusedForGC;

  int faceId;
};

GridFacer *gridfacerCreate(Grid *, int faceId);
Grid *gridfacerGrid(GridFacer *);
void gridfacerFree(GridFacer *);
void gridfacerPack(void *voidGridFacer, 
		  int nnode, int maxnode, int *nodeo2n,
		  int ncell, int maxcell, int *cello2n,
		  int nface, int maxface, int *faceo2n,
		  int nedge, int maxedge, int *edgeo2n);
void gridfacerSortNode(void *voidGridFacer, int maxnode, int *o2n);
void gridfacerReallocator(void *voidGridFacer, int reallocType, 
			 int lastSize, int newSize);
void gridfacerGridHasBeenFreed(void *voidGridFacer );

int gridfacerFaceId(GridFacer *);

GridFacer *gridfacerExaine(GridFacer *);

END_C_DECLORATION

#endif /* GRIDFACER_H */
