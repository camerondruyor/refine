
/* Interp, a list items ranked in priorty
 *
 * Michael A. Park
 * Computational Modeling & Simulation Branch
 * NASA Langley Research Center
 * Phone:(757)864-6604
 * Email:m.a.park@larc.nasa.gov
 */

#include <stdlib.h>
#include <stdio.h>
#ifndef __APPLE__       /* Not needed on Mac OS X */
#include <malloc.h>
#endif
#ifdef __APPLE__       /* Not needed on Mac OS X */
#include <float.h>
#else
#include <values.h>
#endif
#include "interp.h"
#include "gridmath.h"

  /* Rule 4 Solin, Segeth and Dolezel (SSD): order 6 */
static int nq = 24;
static double xq[] = { -0.570794257481696, -0.287617227554912, -0.570794257481696,
		  -0.570794257481696, -0.918652082930777, 0.755956248792332,
		  -0.918652082930777, -0.918652082930777, -0.355324219715449,
		  -0.934027340853653, -0.355324219715449, -0.355324219715449,
		  -0.872677996249965, -0.872677996249965, -0.872677996249965,
		  -0.872677996249965, -0.872677996249965, -0.872677996249965,
		  -0.460655337083368, -0.460655337083368, -0.460655337083368,
		  0.206011329583298, 0.206011329583298, 0.206011329583298 };
static double yq[] = { -0.570794257481696, -0.570794257481696, -0.287617227554912,
		  -0.570794257481696, -0.918652082930777, -0.918652082930777,
		  0.755956248792332, -0.918652082930777, -0.355324219715449,
		  -0.355324219715449, -0.934027340853653, -0.355324219715449,
		  -0.872677996249965, -0.460655337083368, -0.872677996249965,
		  0.206011329583298, -0.460655337083368, 0.206011329583298,
		  -0.872677996249965, -0.872677996249965, 0.206011329583298,
		  -0.872677996249965, -0.872677996249965, -0.460655337083368 };
static double zq[] = { -0.570794257481696, -0.570794257481696, -0.570794257481696,
		  -0.287617227554912, -0.918652082930777, -0.918652082930777,
		  -0.918652082930777, 0.755956248792332, -0.355324219715449,
		  -0.355324219715449, -0.355324219715449, -0.934027340853653,
		  -0.460655337083368, -0.872677996249965, 0.206011329583298,
		  -0.872677996249965, 0.206011329583298, -0.460655337083368,
		  -0.872677996249965, 0.206011329583298, -0.872677996249965,
		  -0.460655337083368, -0.872677996249965, -0.872677996249965 };
static double wq[] = { 0.053230333677557, 0.053230333677557, 0.053230333677557,
		  0.053230333677557, 0.013436281407094, 0.013436281407094,
		  0.013436281407094, 0.013436281407094, 0.073809575391540,
		  0.073809575391540, 0.073809575391540, 0.073809575391540,
		  0.064285714285714, 0.064285714285714, 0.064285714285714,
		  0.064285714285714, 0.064285714285714, 0.064285714285714,
		  0.064285714285714, 0.064285714285714, 0.064285714285714,
		  0.064285714285714, 0.064285714285714, 0.064285714285714 };

static double tet_volume6( double *a, double *b, double *c, double *d )
{
  double m11, m12, m13;
  double det;

  m11 = (a[0]-d[0])*((b[1]-d[1])*(c[2]-d[2])-(c[1]-d[1])*(b[2]-d[2]));
  m12 = (a[1]-d[1])*((b[0]-d[0])*(c[2]-d[2])-(c[0]-d[0])*(b[2]-d[2]));
  m13 = (a[2]-d[2])*((b[0]-d[0])*(c[1]-d[1])-(c[0]-d[0])*(b[1]-d[1]));
  det = ( m11 - m12 + m13 );

  return(-det);
}

Interp* interpCreate( Grid *grid, int function_id, int order, int error_order )
{
  Interp *interp;
  int cell;
  int nodes[4];
  int node;
  double xyz[3];

  interp = (Interp *)malloc( sizeof(Interp) );

  interp->grid = gridDup(grid);
  interp->function_id = function_id;
  interp->error_order = error_order;	
  if ( order < 0 ) {
    interp->f = NULL;
  }else{
    if ( 1 != order ) 
      {
	printf("interpCreate %d order not implemented\n",order);
	return NULL;
      }
    interp->order = EMPTY; /* for temporary exact interp */	
    interp->f = (double *)malloc( 4*gridNCell(interp->grid)*sizeof(double) );
    for(cell=0;cell<gridNCell(interpGrid(interp));cell++)
      {
	gridCell(interpGrid(interp),cell,nodes);
	for(node=0;node<4;node++)
	  {
	    gridNodeXYZ(grid, nodes[node], xyz );
	    interpFunction( interp, xyz, &(interp->f[node+4*cell]) );
	  }
	if (TRUE) {
	  double a[20];
	  int row, col;
	  int iq,j;
	  double xyz0[3];
	  double xyz1[3];
	  double xyz2[3];
	  double xyz3[3];
	  double bary[4];
	  double func;
	  gridNodeXYZ(grid, nodes[0], xyz0 );
	  gridNodeXYZ(grid, nodes[1], xyz1 );
	  gridNodeXYZ(grid, nodes[2], xyz2 );
	  gridNodeXYZ(grid, nodes[3], xyz3 );
	  for(row=0;row<20;row++)a[row]=0.0;
	  for(iq=0;iq<nq;iq++)
	    {
	      bary[1] = 0.5*(1.0+xq[iq]); 
	      bary[2] = 0.5*(1.0+yq[iq]); 
	      bary[3] = 0.5*(1.0+zq[iq]); 
	      bary[0] = 1.0-bary[1]-bary[2]-bary[3];
	      for(j=0;j<3;j++)
		xyz[j] = bary[0]*xyz0[j] + bary[1]*xyz1[j] + 
		  bary[2]*xyz2[j] + bary[3]*xyz3[j];
	      interpFunction( interp, xyz, &func );
	      for(row=0;row<4;row++)
		{
		  for(col=0;col<4;col++)
		    a[row+col*4] += bary[row]*bary[col];
		  a[row+4*4] += bary[row]*func;
		}
	    }
	  gridGaussianElimination( 4, 5, a );
	  gridGaussianBacksolve( 4, 5, a );
	  for(row=0;row<4;row++)interp->f[row+4*cell] = a[row+4*4];
	  
	}
      }
  }
  interp->order = order;
  return interp;
}

void interpFree( Interp *interp )
{
  if ( NULL != interp->f ) free(interp->f);
  if ( NULL != interp->grid ) gridFree(interp->grid);
  free( interp );
}

int interpNB(Interp *interp)
{
  switch ( interpOrder(interp) ) 
    {
    case 1:
      return 4;
      break;
    case 2:
      return 10;
      break;
    default:
      printf("interpNB %d order not translated to nb\n",interpOrder(interp));
      return EMPTY;
    }
}

GridBool interpPhi( Interp *interp, double *bary, double *phi )
{
  int j;
  double x, y, z;
  x = bary[1];
  y = bary[2];
  z = bary[3];
  switch ( interpOrder(interp) ) 
    {
    case 1:
      for(j=0;j<4;j++) phi[j] = bary[j];
      break;
    case 2:
      phi[0] = 1.0-3.0*z-3.0*y-3.0*x+2.0*z*z+4.0*y*z+4.0*x*z+2.0*y*y+4.0*x*y+2.0*x*x;
      phi[1] = -x+2.0*x*x;
      phi[2] = -y+2.0*y*y;
      phi[3] = -z+2.0*z*z;

      phi[4] = 4.0*x-4.0*x*z-4.0*x*y-4.0*x*x;
      phi[5] = 4.0*y-4.0*y*z-4.0*y*y-4.0*x*y;
      phi[6] = 4.0*z-4.0*z*z-4.0*y*z-4.0*x*z;

      phi[7] = 4.0*x*y;
      phi[8] = 4.0*x*z;
      phi[9] = 4.0*y*z;
      break;
    default:
      printf("interpPhi %d order has no phi\n",interpOrder(interp));
      return FALSE;
    }
  return TRUE;
}

GridBool interpLoc2Row( Interp *interp, int cell, int *loc2row )
{
  int edge;
  Grid *grid = interpGrid(interp);
  switch ( interpOrder(interp) ) 
    {
    case 1:
      if ( grid != gridCell(grid,cell,loc2row)) return FALSE;
      break;
    case 2:
      if ( grid != gridCell(grid,cell,loc2row)) return FALSE;
      for(edge=0;edge<6;edge++)
	loc2row[4+edge] = gridNNode(grid) + gridCell2Conn(grid, cell, edge );
      break;
    default:
      printf("interpLoc2Row %d order has no phi\n",interpOrder(interp));
      return FALSE;
    }
  return TRUE;
}

double interpVectProduct( int nrow, double *v1, double *v2 )
{
  double norm;
  int i;

  norm = 0.0;
  for (i=0;i<nrow;i++) norm += v1[i]*v2[i];

  return norm;
}

GridBool interpReconstructB( Interp *orig, Interp *interp, int nrow, double *b )
{
  int cell, nodes[4];
  int iq,j,row;
  double xyz0[3];
  double xyz1[3];
  double xyz2[3];
  double xyz3[3];
  double xyz[3];
  double volume6;
  double bary[4];
  double func;
  int nb;
  int loc2row[10];
  double phi[10];

  Grid *grid = interpGrid(interp);

  for(row=0;row<nrow;row++) b[row] = 0.0;

  for(cell=0;cell<gridNCell(grid);cell++)
    {
      gridCell(grid,cell,nodes);
      gridNodeXYZ(grid, nodes[0], xyz0 );
      gridNodeXYZ(grid, nodes[1], xyz1 );
      gridNodeXYZ(grid, nodes[2], xyz2 );
      gridNodeXYZ(grid, nodes[3], xyz3 );
      volume6 = tet_volume6(xyz0,xyz1,xyz2,xyz3);
      for(iq=0;iq<nq;iq++)
	{
	  bary[1] = 0.5*(1.0+xq[iq]); 
	  bary[2] = 0.5*(1.0+yq[iq]); 
	  bary[3] = 0.5*(1.0+zq[iq]); 
	  bary[0] = 1.0-bary[1]-bary[2]-bary[3];
	  for(j=0;j<3;j++)
	    xyz[j] = bary[0]*xyz0[j] + bary[1]*xyz1[j] + 
	             bary[2]*xyz2[j] + bary[3]*xyz3[j];
	  interpFunction( orig, xyz, &func );
	  nb = interpNB(interp);
	  interpPhi( interp, bary, phi );
	  /*	  for(j=0;j<nb;j++) phi[j] *= 0.125 * volume6 * wq[iq]; */
	  if ( !interpLoc2Row( interp, cell, loc2row )) 
	    return FALSE;
	  for(row=0;row<nb;row++)
	    b[loc2row[row]] += func * phi[row];
	}
    }

  return TRUE;  
}

GridBool interpReconstructAx( Interp *interp, int nrow, double *x, double *ax )
{
  int cell, nodes[4];
  int iq,j,row, col;
  double xyz0[3];
  double xyz1[3];
  double xyz2[3];
  double xyz3[3];
  double volume6;
  double bary[4];
  int nb;
  int loc2row[10];
  double phi[10];

  Grid *grid = interpGrid(interp);

  for(row=0;row<nrow;row++) ax[row] = 0.0;

  for(cell=0;cell<gridNCell(grid);cell++)
    {
      gridCell(grid,cell,nodes);
      gridNodeXYZ(grid, nodes[0], xyz0 );
      gridNodeXYZ(grid, nodes[1], xyz1 );
      gridNodeXYZ(grid, nodes[2], xyz2 );
      gridNodeXYZ(grid, nodes[3], xyz3 );
      volume6 = tet_volume6(xyz0,xyz1,xyz2,xyz3);
      for(iq=0;iq<nq;iq++)
	{
	  bary[1] = 0.5*(1.0+xq[iq]); 
	  bary[2] = 0.5*(1.0+yq[iq]); 
	  bary[3] = 0.5*(1.0+zq[iq]); 
	  bary[0] = 1.0-bary[1]-bary[2]-bary[3];
	  nb = interpNB(interp);
	  interpPhi( interp, bary, phi );
	  /*	  for(j=0;j<nb;j++) phi[j] *= 0.125 * volume6 * wq[iq]; */
	  if ( !interpLoc2Row( interp, cell, loc2row ))
	    return FALSE;
	  for(row=0;row<nb;row++)
	    for(col=0;col<nb;col++)
	      ax[loc2row[row]] += phi[row] * phi[col] * x[loc2row[col]];
	}
    }

  return TRUE;  
}

Interp *interpContinuousReconstruction( Interp *orig, 
					int order, int error_order )
{
  Interp *interp;
  int cell;
  int nodes[4];
  int node;
  double xyz[3];
  double *x, *p, *r, *b, *ap;
  int nrow, nb;
  int loc2row[10];
  int iteration;
  double resid0,resid,last_resid,pap,alpha;

  interp = (Interp *)malloc( sizeof(Interp) );

  interp->grid = gridDup(interpGrid(orig));
  interp->function_id = orig->function_id;
  interp->error_order = error_order;
  interp->order = order;

  nb = interpNB(interp);
  switch ( interpOrder(interp) ) 
    {
    case 1:
      nrow = gridNNode(interpGrid(interp));
      break;
    case 2:
      gridCreateConn(interpGrid(interp));
      gridCreateConn(interpGrid(orig));
      nrow = gridNNode(interpGrid(interp)) + gridNConn(interpGrid(interp));
      break;
    default:
      printf( "interpContinuousReconstruction %d order not implemented\n",
	      interpOrder(interp));
      return NULL;
    }
  x  = (double *)malloc( nrow*sizeof(double) );
  p  = (double *)malloc( nrow*sizeof(double) );
  r  = (double *)malloc( nrow*sizeof(double) );
  b  = (double *)malloc( nrow*sizeof(double) );
  ap = (double *)malloc( nrow*sizeof(double) );
  
  interpReconstructB( orig, interp, nrow, b );
  for(node=0;node<nrow;node++) x[node] = 0.0;
  for(node=0;node<nrow;node++) r[node] = b[node];
  for(node=0;node<nrow;node++) p[node] = r[node];

  resid = interpVectProduct( nrow, r, r );
  printf("resid0 %e\n",resid);
  resid0 = resid;
  for (iteration =0; ((iteration<100)&&((resid/resid0)>1.0e-15)); iteration++)
    {
      interpReconstructAx( interp, nrow, p, ap );
      pap = interpVectProduct( nrow, p, ap );
      alpha = resid/pap;
      for(node=0;node<nrow;node++) x[node] += alpha*p[node];
      for(node=0;node<nrow;node++) r[node] -= alpha*ap[node];
      last_resid = resid;
      resid = interpVectProduct( nrow, r, r );
      printf("%3d %e\n",iteration+1,resid/resid0);
      for(node=0;node<nrow;node++) 
	p[node] = r[node] + resid/last_resid*p[node];
    }

  free(p);
  free(r);
  free(b);
  free(ap);

  interp->f = (double *)malloc( nb*gridNCell(interp->grid)*sizeof(double) );
  for(cell=0;cell<gridNCell(interpGrid(interp));cell++)
    {
      gridCell(interpGrid(interp),cell,nodes);
      if ( !interpLoc2Row( interp, cell, loc2row )) return FALSE;
      for(node=0;node<nb;node++)
	  interp->f[node+nb*cell] = x[loc2row[node]];
    }
  free(x);
  return interp;
}

GridBool interpFunction( Interp *interp, double *xyz, double *func )
{
  double a, c, tanhaz;
  double bary[4];
  int cell;
  if ( interpOrder(interp) > 0 ) {
    cell = gridFindEnclosingCell(interpGrid(interp), 0, xyz, bary);
    if ( EMPTY == cell )
      {
	printf("%s: %d: gridFindEnclosingCell failed in %s\n",
	       __FILE__,__LINE__,__func__);
	return FALSE;
      }
    return interpFunctionInCell( interp, cell, bary, func );
  } else {
    switch (interpFunctionId(interp)) {
    case 0:
      (*func) = 8.0*xyz[0]*xyz[0] + 32.0*xyz[1]*xyz[1] + 128.0*xyz[2]*xyz[2];
      break;
    case 1:
      a = 10.0;
      c = 1000.0;
      (*func) = 8.0*xyz[0]*xyz[0] + 32.0*xyz[1]*xyz[1] +
	c*tanh(a*(xyz[2]-0.5));
      break;
    }
  }

  return TRUE;
}

GridBool interpFunctionInCell( Interp *interp, 
			       int cell, double *bary, double *func )
{
  int nb, node;
  double phi[10];
  (*func) = 0.0;
  nb = interpNB(interp);
  if (EMPTY == nb) {
    printf(" nb %d in interpFunctionInCell\n",nb);
    return FALSE;
  }
  if (!interpPhi( interp, bary, phi ) ) return FALSE;
  for(node=0;node<nb;node++)
    (*func) += interp->f[node+nb*cell]*phi[node];
  return TRUE;
}

GridBool interpMetric( Interp *interp, double *xyz, double *m )
{
  double a, c, tanhaz;
  switch (interpFunctionId(interp)) {
  case 0:
    m[0] =  16.0;
    m[1] =   0.0;
    m[2] =   0.0;
    m[3] =  64.0;
    m[4] =   0.0;
    m[5] = 256.0;
    break;
  case 1:
    a = 10.0;
    c = 1000.0;
    m[0] =  16.0;
    m[1] =   0.0;
    m[2] =   0.0;
    m[3] =  64.0;
    m[4] =   0.0;
    tanhaz = tanh(a*(xyz[2]-0.5));
    m[5] = -c*a*2.0*tanhaz*(1.0-tanhaz*tanhaz)*a;
    m[5] = MAX(ABS(m[5]),1.0);
    break;
  }
    
  return TRUE;
}

GridBool interpError( Interp *interp,
		      double *xyz0, double *xyz1, double *xyz2, double *xyz3, 
		      double *error )
{
  int i,j;
  double xyz[3];
  double b0,b1,b2,b3;
  double pinterp, func;
  double phi;
  double n0,n1,n2,n3;
  double e01,e02,e03,e12,e13,e23;
  double diff,volume6;

  volume6 = tet_volume6(xyz0,xyz1,xyz2,xyz3);
  if ( volume6 <= 6.0e-12 ) {
    if (ABS(volume6)< 1.0e-15) volume6 = 1.0e-15;
    (*error) = 1.0/ABS(volume6);
    return TRUE; 
  }

  (*error) = 0.0;
  interpFunction( interp, xyz0, &n0 );
  interpFunction( interp, xyz1, &n1 );
  interpFunction( interp, xyz2, &n2 );
  interpFunction( interp, xyz3, &n3 );
  if ( 2 == interpErrorOrder(interp) )
    {
      gridAverageVector(xyz0,xyz1,xyz); interpFunction( interp, xyz, &e01 );
      gridAverageVector(xyz0,xyz2,xyz); interpFunction( interp, xyz, &e02 );
      gridAverageVector(xyz0,xyz3,xyz); interpFunction( interp, xyz, &e03 );
      gridAverageVector(xyz1,xyz2,xyz); interpFunction( interp, xyz, &e12 );
      gridAverageVector(xyz1,xyz3,xyz); interpFunction( interp, xyz, &e13 );
      gridAverageVector(xyz2,xyz3,xyz); interpFunction( interp, xyz, &e23 );
    }

  for(i=0;i<nq;i++)
    {
      b1 = 0.5*(1.0+xq[i]); 
      b2 = 0.5*(1.0+yq[i]); 
      b3 = 0.5*(1.0+zq[i]); 
      b0 = 1.0-b1-b2-b3;
      for(j=0;j<3;j++)
	xyz[j] = b0*xyz0[j] + b1*xyz1[j] + b2*xyz2[j] + b3*xyz3[j];
      interpFunction( interp, xyz, &func );
      switch ( interpErrorOrder(interp) ) 
	{
	case 1:
	  pinterp = b0*n0 + b1*n1 + b2*n2 + b3*n3; break;
	case 2:
	  pinterp = 0.0;
	  phi = 1.0-3.0*b3-3.0*b2-3.0*b1+2.0*b3*b3+4.0*b2*b3+4.0*b1*b3+2.0*b2*b2+4.0*b1*b2+2.0*b1*b1;
	  pinterp += phi*n0;
	  phi = 4.0*b1-4.0*b1*b3-4.0*b1*b2-4.0*b1*b1;
	  pinterp += phi*e01;
	  phi = -b1+2.0*b1*b1;
	  pinterp += phi*n1;
	  phi = 4.0*b2-4.0*b2*b3-4.0*b2*b2-4.0*b1*b2;
	  pinterp += phi*e02;
	  phi = 4.0*b1*b2;
	  pinterp += phi*e12;
	  phi = -b2+2.0*b2*b2;
	  pinterp += phi*n2;
	  phi = 4.0*b3-4.0*b3*b3-4.0*b2*b3-4.0*b1*b3;
	  pinterp += phi*e03;
	  phi = 4.0*b1*b3;
	  pinterp += phi*e13;
	  phi = 4.0*b2*b3;
	  pinterp += phi*e23;
	  phi = -b3+2.0*b3*b3;
	  pinterp += phi*n3;
	  break;
	default:
	  printf("%s: %d: interpError: error_order %d not implemented\n",
		 __FILE__,__LINE__,interpErrorOrder(interp));
	  return FALSE;
	}
      diff = pinterp-func;
      (*error) += 0.125 * volume6 * wq[i] * diff * diff;
    }

  (*error) = sqrt(*error);

  return TRUE;
}

static int nq1 = 6;
static double tq1[] = {
  0.0337652428984240,
  0.1693953067668678,
  0.3806904069584016,
  0.6193095930415985,
  0.8306046932331322,
  0.9662347571015760 };
static double wq1[] = {
  0.0856622461895852,
  0.1803807865240693,
  0.2339569672863455,
  0.2339569672863455,
  0.1803807865240693,
  0.0856622461895852 };

GridBool interpError1D( Interp *interp,
			double *xyz0, double *xyz1, 
			double *error )
{
  double n0, n1;
  double xyz[3];
  double length;
  int iq, j;
  double t;
  double pinterp, func;
  double diff;

  if ( 1 != interpErrorOrder(interp) )
    {
      printf("%s: %d: interpErroriD: error_order %d not implemented\n",
	     __FILE__,__LINE__,interpErrorOrder(interp));
    }

  (*error) = 0.0;
  interpFunction( interp, xyz0, &n0 );
  interpFunction( interp, xyz1, &n1 );
  gridSubtractVector(xyz1,xyz0,xyz);
  length = gridVectorLength(xyz);
  for(iq=0;iq<nq1;iq++)
    {
      t = tq1[iq];
      for(j=0;j<3;j++)
	xyz[j] = t*xyz1[j] + (1.0-t)*xyz0[j];
      pinterp = t*n1 + (1.0-t)*n0;
      interpFunction( interp, xyz, &func );
      diff = pinterp-func;
      (*error) += length * wq1[iq] * diff * diff;
    }

  (*error) = sqrt(*error);

  return TRUE;
}

GridBool interpSplitImprovement1D( Interp *interp, 
				   double *xyz0, double *xyz1,
				   double *error_before, double *error_after )
{
  double mid[3];
  double error;

  if ( !interpError1D( interp,xyz0,xyz1,error_before ) ) return FALSE;
  gridAverageVector(xyz0,xyz1,mid);

  if ( !interpError1D( interp,xyz0,mid,&error ) ) return FALSE;
  (*error_after) = error*error;
  if ( !interpError1D( interp,mid,xyz1,&error ) ) return FALSE;
  (*error_after) += error*error;

  (*error_after) = sqrt(*error_after);

  return TRUE;
}

GridBool interpSplitImprovement( Interp *interp, 
				   double *xyz0, double *xyz1,
				   double *xyz2, double *xyz3,
				   double *error_before, double *error_after )
{
  double mid[3];
  double error;

  if ( !interpError( interp,xyz0,xyz1,xyz2,xyz3,error_before ) ) return FALSE;
  gridAverageVector(xyz0,xyz1,mid);

  if ( !interpError( interp,mid,xyz1,xyz2,xyz3,&error ) ) return FALSE;
  (*error_after) = error*error;
  if ( !interpError( interp,xyz0,mid,xyz2,xyz3,&error ) ) return FALSE;
  (*error_after) += error*error;

  (*error_after) = sqrt(*error_after);

  return TRUE;
}

GridBool interpTecplot( Interp *interp, char *filename )
{

  FILE *f;
  int nodes[3], face, faceId;
  int cell, cellnodes[4];
  double xyz0[3],xyz1[3],xyz2[3],xyz[3];
  double cell0[3],cell1[3],cell2[3],cell3[3];
  double bary[4];
  double val;
  Grid *grid = interpGrid(interp);

  if (NULL == filename)
    {
      f = fopen( "interp.t", "w" );
    } else {
      f = fopen( filename, "w" );
    }

  if ( NULL == f ) return FALSE;

  fprintf( f, "title=\"tecplot interp file\"\n" );
  fprintf( f, "variables=\"x\",\"y\",\"z\",\"s\"\n" );

  fprintf( f, "zone t=surf, i=%d, j=%d, f=fepoint, et=triangle\n",
	   6*gridNFace(grid), 4*gridNFace(grid));

  for (face=0;face<gridMaxFace(grid);face++)
    if ( gridFace(grid, face, nodes, &faceId) )
      if ( interpOrder(interp) > 0 ) {
	cell = gridFindCellWithFace(grid, face );
	if ( EMPTY == cell ) return FALSE;
	gridCell(grid,cell,cellnodes);
	gridNodeXYZ(grid, cellnodes[0], cell0 );
	gridNodeXYZ(grid, cellnodes[1], cell1 );
	gridNodeXYZ(grid, cellnodes[2], cell2 );
	gridNodeXYZ(grid, cellnodes[3], cell3 );
	gridNodeXYZ(grid, nodes[0], xyz0 );
	gridBarycentricCoordinate(cell0, cell1, cell2, cell3, xyz0, bary );
	interpFunctionInCell( interp, cell, bary, &val );
	fprintf( f, "%25.17e %25.17e %25.17e %25.17e\n",
		 xyz0[0], xyz0[1], xyz0[2], val );	
	gridNodeXYZ(grid, nodes[1], xyz1 );
	gridBarycentricCoordinate(cell0, cell1, cell2, cell3, xyz1, bary );
	interpFunctionInCell( interp, cell, bary, &val );
	fprintf( f, "%25.17e %25.17e %25.17e %25.17e\n",
		 xyz1[0], xyz1[1], xyz1[2], val );
	gridNodeXYZ(grid, nodes[2], xyz2 );
	gridBarycentricCoordinate(cell0, cell1, cell2, cell3, xyz2, bary );
	interpFunctionInCell( interp, cell, bary, &val );
	fprintf( f, "%25.17e %25.17e %25.17e %25.17e\n",
		 xyz2[0], xyz2[1], xyz2[2], val );
	gridAverageVector(xyz1,xyz2,xyz)
	gridBarycentricCoordinate(cell0, cell1, cell2, cell3, xyz, bary );
	interpFunctionInCell( interp, cell, bary, &val );
	fprintf( f, "%25.17e %25.17e %25.17e %25.17e\n",
		 xyz[0], xyz[1], xyz[2], val );
	gridAverageVector(xyz0,xyz2,xyz)
	gridBarycentricCoordinate(cell0, cell1, cell2, cell3, xyz, bary );
	interpFunctionInCell( interp, cell, bary, &val );
	fprintf( f, "%25.17e %25.17e %25.17e %25.17e\n",
		 xyz[0], xyz[1], xyz[2], val );
	gridAverageVector(xyz0,xyz1,xyz)
	gridBarycentricCoordinate(cell0, cell1, cell2, cell3, xyz, bary );
	interpFunctionInCell( interp, cell, bary, &val );
	fprintf( f, "%25.17e %25.17e %25.17e %25.17e\n",
		 xyz[0], xyz[1], xyz[2], val );
      }else{
	gridNodeXYZ(grid, nodes[0], xyz0 );
	interpFunction( interp, xyz0, &val );
	fprintf( f, "%25.17e %25.17e %25.17e %25.17e\n",
		 xyz0[0], xyz0[1], xyz0[2], val );
	gridNodeXYZ(grid, nodes[1], xyz1 );
	interpFunction( interp, xyz1, &val );
	fprintf( f, "%25.17e %25.17e %25.17e %25.17e\n",
		 xyz1[0], xyz1[1], xyz1[2], val );
	gridNodeXYZ(grid, nodes[2], xyz2 );
	interpFunction( interp, xyz2, &val );
	fprintf( f, "%25.17e %25.17e %25.17e %25.17e\n",
		 xyz2[0], xyz2[1], xyz2[2], val );
	gridAverageVector(xyz1,xyz2,xyz);
	interpFunction( interp, xyz, &val );
	fprintf( f, "%25.17e %25.17e %25.17e %25.17e\n",
		 xyz[0], xyz[1], xyz[2], val );
	gridAverageVector(xyz0,xyz2,xyz);
	interpFunction( interp, xyz, &val );
	fprintf( f, "%25.17e %25.17e %25.17e %25.17e\n",
		 xyz[0], xyz[1], xyz[2], val );
	gridAverageVector(xyz0,xyz1,xyz);
	interpFunction( interp, xyz, &val );
	fprintf( f, "%25.17e %25.17e %25.17e %25.17e\n",
		 xyz[0], xyz[1], xyz[2], val );
      }

  for (face=0;face<gridNFace(grid);face++)
    {
      fprintf( f, "%d %d %d\n",6*face+1,6*face+6,6*face+5);
      fprintf( f, "%d %d %d\n",6*face+2,6*face+4,6*face+6);
      fprintf( f, "%d %d %d\n",6*face+3,6*face+5,6*face+4);
      fprintf( f, "%d %d %d\n",6*face+4,6*face+5,6*face+6);
    }
    
  fclose(f);

  return TRUE;  
}

double interpTotalError(Grid *grid) 
{
  int cell, nodes[4];
  double xyz0[3], xyz1[3], xyz2[3], xyz3[3];
  double total_error, cell_error;
  total_error = 0.0;
  Interp *interp= gridInterp(grid);
  for (cell=0;cell<gridMaxCell(grid);cell++){ 
    if (grid==gridCell(grid, cell, nodes)) { 
      gridNodeXYZ(grid,nodes[0],xyz0);
      gridNodeXYZ(grid,nodes[1],xyz1);
      gridNodeXYZ(grid,nodes[2],xyz2);
      gridNodeXYZ(grid,nodes[3],xyz3);
      interpError( interp,
		   xyz0,xyz1,xyz2,xyz3, 
		   &cell_error );
      total_error += cell_error*cell_error;
    }
  }
  return sqrt(total_error);
}
