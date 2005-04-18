
/* Michael A. Park
 * Computational Modeling & Simulation Branch
 * NASA Langley Research Center
 * Phone:(757)864-6604
 * Email:m.a.park@larc.nasa.gov
 */

/* $Id$ */

#ifndef GRIDFORTRAN_H
#define GRIDFORTRAN_H

#include "refine_defs.h"

BEGIN_C_DECLORATION

void gridcreate_( int *partId, int *nnode, double *x, double *y, double *z,
		 int *ncell, int *maxcell, int *c2n );
void gridfree_( void );
void gridinsertboundary_( int *faceId, int *nnode, int *nodedim, int *inode, 
			 int *nface, int *dim1, int *dim2, int *f2n );
void gridsetmap_( int *nnode, double* map );
void gridsetnodelocal2global_( int *partId, int *nnodeg, 
			      int *nnode, int *nnode0, int *local2global );
void gridsetnodepart_( int *nnode, int *part );
void gridsetcelllocal2global_( int *ncellg, int *ncell, int *local2global );
void gridfreezenode_( int *node );
void gridparallelloadcapri_( char *url, char *modeler, char *capriProject,
                             int *status );
void gridprojectallfaces_( void );
void gridtestcadparameters_( void );
void gridminar_( double *aspectratio );
void gridwritetecplotsurfacezone_( void );
void gridexportfast_( void );

void gridparallelswap_( int *processor, double *ARlimit );
void gridparallelsmooth_( int *processor,
			  double *optimizationLimit, double *laplacianLimit,
                          int *geometryAllowed );
void gridparallelrelaxneg_( int *processor, int *geometryAllowed );
void gridparallelrelaxsurf_( int *processor );
void gridparalleladapt_( int *processor, 
			 double *minLength, double *maxLength );
void queuedumpsize_( int *nInt, int *nDouble );
void queuedump_( int *nInt, int *nDouble, int *ints, double *doubles );
void gridapplyqueue_( int *nInt, int *nDouble, int *ints, double *doubles );

void gridsize_( int *nnodeg, int *ncellg );
void gridglobalshift_( int *oldnnodeg, int *newnnodeg, int *nodeoffset,
		      int *oldncellg, int *newncellg, int *celloffset );
void gridrenumberglobalnodes_( int *nnode, int *new2old );

void gridnunusednodeglobal_( int *nunused );
void gridgetunusednodeglobal_( int *nunused, int *unused );
void gridjoinunusednodeglobal_( int *nunused, int *unused );
void grideliminateunusednodeglobal_( void );
void gridnunusedcellglobal_( int *nunused );
void gridgetunusedcellglobal_( int *nunused, int *unused );
void gridjoinunusedcellglobal_( int *nunused, int *unused );
void grideliminateunusedcellglobal_( void );

void gridsortfun3d_( int *nnodes0, int *nnodes01, int *nnodesg, 
		    int *ncell, int *ncellg );
void gridgetnodes_( int *nnode, int *l2g, double *x, double *y, double *z);
void gridgetcell_( int *cell, int *nodes, int *global );
void gridgetbcsize_( int *ibound, int *nface );
void gridgetbc_( int *ibound, int *nface, int *ndim, int *f2n );

void gridsetnaux_( int *naux );
void gridsetauxvector_( int *nnode, int *offset, double *x );
void gridsetauxmatrix_( int *ndim, int *nnode, int *offset, double *x );
void gridsetauxmatrix3_( int *ndim, int *nnode, int *offset, double *x );
void gridgetauxvector_( int *nnode, int *offset, double *x );
void gridgetauxmatrix_( int *ndim, int *nnode, int *offset, double *x );
void gridgetauxmatrix3_( int *ndim, int *nnode, int *offset, double *x );

void gridghostcount_( int *nproc, int *count );

void gridloadghostnodes_( int *nproc, int *clientindex,
			 int *clientsize, int *localnode, int *globalnode );
void gridloadglobalnodedata_( int *ndim, int *nnode, int *nodes, double *data );
void gridloadlocalnodes_( int *nnode, int *global, int *local );
void gridsetlocalnodedata_( int *ndim, int *nnode, int *nodes, double *data );

void gridcopyabouty0_( int *symmetryFaceId, int *mirrorAux );

void gridmovesetprojectiondisp_( void );
void gridmoverelaxstartup_( int *relaxationScheme );
void gridmoverelaxstartstep_( double *position); 
void gridmoverelaxsubiter_( double *residual);
void gridmoverelaxshutdown_( void );
void gridmoveapplydisplacements_( void );

void gridmovedataleadingdim_( int *ndim );
void gridmoveinitializempitest_( void );
void gridmovecompletempitest_( void );
void gridmoveloadlocalnodedata_( int *ndim, int *nnode, 
				 int *nodes, double *data );
void gridmovesetlocalnodedata_( int *ndim, int *nnode, 
				int *nodes, double *data );

void gridmovefree_( void );

void gridgeomsize_( int *nGeomNode, int *nGeomEdge, int *nGeomFace );
void gridlocalboundnode_( int *nBoundNode );
void gridgeomedgeendpoints_( int *edgeId, int *endPoints );
void gridmaxedge_( int *maxedge );
void gridedge_( int *edge, int *edgeId, 
		int *globalnodes, int *nodeparts, 
		double *t, double *xyz);
void gridupdateedgegrid_(int *edgeId, int *nCurveNode, double *xyz, double *t);
void gridmaxface_( int *maxface );
void gridface_( int *face, int *faceId, 
		int *globalnodes, int *nodeparts, 
		double *uv, double *xyz);
void gridfaceedgecount_( int *faceId, int *faceEdgeCount );
void gridfaceedgel2g_( int *faceId, int *faceEdgeCount, int *local2global );

void gridupdategeometryface_( int *faceId, int *nnode, double *xyz, double *uv,
			      int *nface, int *f2n );

void gridcreateshellfromfaces_( void );

END_C_DECLORATION

#endif /* GRIDFORTRAN_H */
