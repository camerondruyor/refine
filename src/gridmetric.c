#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <values.h>
#include "adj.h"
#include "gridmetric.h"
#include "gridStruct.h"

#define VECTOR_COPY3(a,b) for(i=0;i<3;i++)a[i]=b[i];

Grid *gridMapMatrix(Grid *grid, int node, double *m)
{
  m[0] = grid->map[0+6*node];
  m[1] = grid->map[1+6*node];
  m[2] = grid->map[2+6*node];
  m[3] = grid->map[3+6*node];
  m[4] = grid->map[4+6*node];
  m[5] = grid->map[5+6*node];
  
  return grid;
}

void gridMapXYZWithJ( double *j,
		      double *x, double *y, double *z )
{
  double mapx, mapy, mapz;
  
  mapx = j[0] * *x + j[1] * *y + j[2] * *z; 
  mapy = j[3] * *x + j[4] * *y + j[5] * *z; 
  mapz = j[6] * *x + j[6] * *y + j[8] * *z; 

  *x = mapx;
  *y = mapy;
  *z = mapz;
}

double gridEdgeLength(Grid *grid, int n0, int n1 )
{
  double dx, dy, dz;

  dx = grid->xyz[0+3*n1] - grid->xyz[0+3*n0];
  dy = grid->xyz[1+3*n1] - grid->xyz[1+3*n0];
  dz = grid->xyz[2+3*n1] - grid->xyz[2+3*n0];

  return  sqrt(dx*dx+dy*dy+dz*dz);
}

double gridEdgeRatio(Grid *grid, int n0, int n1 )
{
  double dx, dy, dz;
  double m[6];
  double length0, length1;

  dx = grid->xyz[0+3*n1] - grid->xyz[0+3*n0];
  dy = grid->xyz[1+3*n1] - grid->xyz[1+3*n0];
  dz = grid->xyz[2+3*n1] - grid->xyz[2+3*n0];

  gridMapMatrix(grid, n0, m);
  length0 = sqrt (
      dx * ( m[0]*dx + m[1]*dy + m[2]*dz )
    + dy * ( m[1]*dx + m[3]*dy + m[4]*dz )
    + dz * ( m[2]*dx + m[4]*dy + m[5]*dz ) );

  gridMapMatrix(grid, n1, m);
  length1 = sqrt (
      dx * ( m[0]*dx + m[1]*dy + m[2]*dz )
    + dy * ( m[1]*dx + m[3]*dy + m[4]*dz )
    + dz * ( m[2]*dx + m[4]*dy + m[5]*dz ) );
  
  return 0.5*(length0+length1);
}

double gridAverageEdgeLength(Grid *grid, int node )
{
  AdjIterator it;
  int ncell, cell, nodes[4];
  double length, celllength;
  ncell = 0;
  length = 0.0;
  for ( it = adjFirst(grid->cellAdj,node); adjValid(it); it = adjNext(it) ){
    ncell++;
    cell = adjItem(it);
    gridCell( grid, cell, nodes);
    celllength 
      = gridEdgeLength( grid, nodes[0], nodes[1] )
      + gridEdgeLength( grid, nodes[0], nodes[2] )
      + gridEdgeLength( grid, nodes[0], nodes[3] )
      + gridEdgeLength( grid, nodes[1], nodes[2] )
      + gridEdgeLength( grid, nodes[1], nodes[3] )
      + gridEdgeLength( grid, nodes[2], nodes[3] ) ;
    length += celllength/6.0;      
  }

  return length/(double)ncell;
}

Grid *gridLargestRatioEdge(Grid *grid, int node, 
			   int *edgeNode, double *ratio )
{
  AdjIterator it;
  int i, cell, nodes[4];
  double currentRatio;
  
  *edgeNode = EMPTY;
  *ratio = -1.0;
  for ( it = adjFirst(grid->cellAdj,node); adjValid(it); it = adjNext(it) ){
    cell = adjItem(it);
    gridCell( grid, cell, nodes);
    for (i=0;i<4;i++){
      if (node != nodes[i]) {
	currentRatio = gridEdgeRatio( grid, node, nodes[i] );;
	if ( currentRatio > *ratio ){
	  *ratio = currentRatio;
	  *edgeNode = nodes[i];
	}
      }
    }
  }
  
  return grid;
}

Grid *gridSmallestRatioEdge(Grid *grid, int node, 
			    int *edgeNode, double *ratio )
{
  AdjIterator it;
  int i, cell, nodes[4];
  double currentRatio;
  
  *edgeNode = EMPTY;
  *ratio = DBL_MAX;
  for ( it = adjFirst(grid->cellAdj,node); adjValid(it); it = adjNext(it) ){
    cell = adjItem(it);
    gridCell( grid, cell, nodes);
    for (i=0;i<4;i++){
      if (node != nodes[i]) {
	currentRatio = gridEdgeRatio( grid, node, nodes[i] );;
	if ( currentRatio < *ratio ){
	  *ratio = currentRatio;
	  *edgeNode = nodes[i];
	}
      }
    }
  }
  
  return grid;
}

double gridSpacing(Grid *grid, int node )
{
  return 1.0/sqrt(grid->map[0+6*node]);
}

Grid *gridResetSpacing(Grid *grid )
{
  int node;
  double spacingInverse;
  for ( node=0; node < grid->nnode; node++) { 
    spacingInverse = 1.0/gridAverageEdgeLength( grid, node );
    spacingInverse = spacingInverse * spacingInverse;
    grid->map[0+6*node] = spacingInverse; 
    grid->map[3+6*node] = spacingInverse; 
    grid->map[5+6*node] = spacingInverse;
  } 
  return grid;
}

Grid *gridScaleSpacing(Grid *grid, int node, double scale )
{
  grid->map[0+6*node] = grid->map[0+6*node]/(scale*scale);
  grid->map[3+6*node] = grid->map[3+6*node]/(scale*scale);
  grid->map[5+6*node] = grid->map[5+6*node]/(scale*scale);
  return grid;
}

Grid *gridScaleSpacingSphere( Grid *grid, 
			      double x, double y, double z, double r,
			      double scale )
{
  int node;
  double dx, dy, dz, distanceSquared, radiusSquared;
  radiusSquared = r*r;
  
  for ( node=0; node<grid->nnode; node++ ) {
    dx = grid->xyz[0+3*node] - x;
    dy = grid->xyz[1+3*node] - y;
    dz = grid->xyz[2+3*node] - z;
    distanceSquared = dx*dx + dy*dy + dz*dz;
    if (radiusSquared >= distanceSquared) gridScaleSpacing(grid, node, scale );
  }

  return grid;
}

Grid *gridScaleSpacingSphereDirection( Grid *grid, 
			      double x, double y, double z, double r,
			      double scalex, double scaley, double scalez )
{
  int node;
  double dx, dy, dz, distanceSquared, radiusSquared;
  radiusSquared = r*r;
  
  for ( node=0; node<grid->nnode; node++ ) {
    dx = grid->xyz[0+3*node] - x;
    dy = grid->xyz[1+3*node] - y;
    dz = grid->xyz[2+3*node] - z;
    distanceSquared = dx*dx + dy*dy + dz*dz;
    if (radiusSquared >= distanceSquared){
      grid->map[0+6*node] = grid->map[0+6*node] / (scalex*scalex); 
      grid->map[3+6*node] = grid->map[3+6*node] / (scaley*scaley); 
      grid->map[5+6*node] = grid->map[5+6*node] / (scalez*scalez); 
    } 
  }

  return grid;
}
Grid *gridSetMap(Grid *grid, int node,
		 double m11, double m12, double m13,
		             double m22, double m23,
		                         double m33)
{
  if ( !gridValidNode(grid, node) ) return NULL;

  grid->map[0+6*node] = m11;
  grid->map[1+6*node] = m12;
  grid->map[2+6*node] = m13;
  grid->map[3+6*node] = m22;
  grid->map[4+6*node] = m23;
  grid->map[5+6*node] = m33;

  return grid;
}

Grid *gridEigenValues(Grid *grid, double *m, double *eigenValues)
{
  double t, b, c, d;
  double Q, Rp, Q3, Qr2m, b3;
  double diagRatio;

  diagRatio = (ABS(m[1])+ABS(m[2])+ABS(m[4])) 
    / (ABS(m[0])+ABS(m[3])+ABS(m[5]));

  if (diagRatio < 1.0e-5) {
    eigenValues[0]=m[0];
    eigenValues[1]=m[3];
    eigenValues[2]=m[5];
  }else{
    b = -(m[0] + m[3] + m[5]);
    c = m[0]*m[3] + m[3]*m[5] + m[5]*m[0]
      - m[4]*m[4] - m[2]*m[2] - m[1]*m[1];
    d = m[0]*m[4]*m[4] + m[3]*m[2]*m[2] + m[5]*m[1]*m[1]
      - m[0]*m[3]*m[5] - 2.0*m[4]*m[2]*m[1];

    Q = (b*b - 3.0*c)/9.0;
    Rp = (2.0*b*b*b - 9.0*b*c + 27.0*d)/54.0;
    Q3 = Q*Q*Q;
    if ( Rp*Rp > Q3 ) {
      printf("%s: %d: gridEigenValues: complex roots\n",
	     __FILE__,__LINE__);
      return NULL;
    }
    
    b3 = b/3.0;
    t = acos(Rp/sqrt(Q3));
    Qr2m = -2.0*sqrt(Q);
    eigenValues[0] = Qr2m*cos(t/3.0) - b3;
    eigenValues[1] = Qr2m*cos((t + 2.0*M_PI)/3.0) - b3;
    eigenValues[2] = Qr2m*cos((t - 2.0*M_PI)/3.0) - b3;
  }

  if ( eigenValues[1] > eigenValues[0] ) {
    t = eigenValues[0];
    eigenValues[0] = eigenValues[1];
    eigenValues[1] = t;
  }

  if ( eigenValues[2] > eigenValues[0] ) {
    t = eigenValues[0];
    eigenValues[0] = eigenValues[2];
    eigenValues[2] = t;
  }

  if ( eigenValues[2] > eigenValues[1] ) {
    t = eigenValues[1];
    eigenValues[1] = eigenValues[2];
    eigenValues[2] = t;
  }

  return grid;
}

Grid *gridEigenVector(Grid *grid, double *m, double eigenValue, 
		      double *eigenVector )
{
  double a, d, f, n1, n2, n3;
  double e1[3], e2[3], e3[3];
 
  a = m[0]-eigenValue;
  d = m[3]-eigenValue;
  f = m[5]-eigenValue;

  e1[0] = m[2]*d - m[1]*m[4];
  e1[1] = a*m[4] - m[1]*m[2];
  e1[2] = m[1]*m[1] - a*d;
  n1 = e1[0]*e1[0] + e1[1]*e1[1] + e1[2]*e1[2];
  e2[0] = d*f - m[4]*m[4];
  e2[1] = m[2]*m[4] - m[1]*f;
  e2[2] = m[1]*m[4] - m[2]*d;
  n2 = e2[0]*e2[0] + e2[1]*e2[1] + e2[2]*e2[2];
  e3[0] = m[1]*f - m[2]*m[4];
  e3[1] = m[2]*m[2] - a*f;
  e3[2] = a*m[4] - m[1]*m[2];
  n3 = e3[0]*e3[0] + e3[1]*e3[1] + e3[2]*e3[2];

   if ( n1 > n2 && n1 > n3 )
   {
      n1 = sqrt(n1);
      eigenVector[0] = e1[0]/n1;
      eigenVector[1] = e1[1]/n1;
      eigenVector[2] = e1[2]/n1;
   }
   else if ( n2 > n3 )
   {
      n2 = sqrt(n2);
      eigenVector[0] = e2[0]/n2;
      eigenVector[1] = e2[1]/n2;
      eigenVector[2] = e2[2]/n2;
   }
   else
   {
     if ( n3 == 0.0 ) {
       printf("%s: %d: gridEigenVector: all vectors have zero length\n",
	      __FILE__,__LINE__);
       return NULL;
     }
      n3 = sqrt(n3);
      eigenVector[0] = e3[0]/n3;
      eigenVector[1] = e3[1]/n3;
      eigenVector[2] = e3[2]/n3;
   }
  
  return grid;
}

Grid *gridEigenSystem(Grid *grid, double *m, double *eigenValues,
		      double *v1, double *v2, double *v3)
{
  int i;
  double t, vt[3], n[3];
  double diagRatio;

  diagRatio = (ABS(m[1])+ABS(m[2])+ABS(m[4])) 
    / (ABS(m[0])+ABS(m[3])+ABS(m[5]));

  if (diagRatio < 1.0e-5) {
    eigenValues[0]=m[0];
    eigenValues[1]=m[3];
    eigenValues[2]=m[5];
    v1[0] = 1.0;
    v1[1] = 0.0;
    v1[2] = 0.0;
    v2[0] = 0.0;
    v2[1] = 1.0;
    v2[2] = 0.0;
    v3[0] = 0.0;
    v3[1] = 0.0;
    v3[2] = 1.0;
  }else{
    if ( grid != gridEigenValues( grid, m, eigenValues ) ) {
      printf("%s: %d: gridEigenSystem: gridEigenValues NULL\n",
	     __FILE__,__LINE__);
      return NULL;
    }
    if ( grid != gridEigenVector( grid, m, eigenValues[0], v1 ) ) {
      printf("%s: %d: gridEigenSystem: gridEigenVector 0 NULL\n",
	     __FILE__,__LINE__);
      return NULL;
    }
    if ( grid != gridEigenVector( grid, m, eigenValues[1], v2 ) ) {
      printf("%s: %d: gridEigenSystem: gridEigenVector 1 NULL\n",
	     __FILE__,__LINE__);
      return NULL;
    }
    if ( grid != gridEigenVector( grid, m, eigenValues[2], v3 ) ) {
      printf("%s: %d: gridEigenSystem: gridEigenVector 2 NULL\n",
	     __FILE__,__LINE__);
      return NULL;
    }
  }

  if ( eigenValues[1] > eigenValues[0] ) {
    t = eigenValues[0];
    VECTOR_COPY3(vt,v1);
    eigenValues[0] = eigenValues[1];
    VECTOR_COPY3(v1,v2);
    eigenValues[1] = t;
    VECTOR_COPY3(v2,vt);
  }

  if ( eigenValues[2] > eigenValues[0] ) {
    t = eigenValues[0];
    VECTOR_COPY3(vt,v1);
    eigenValues[0] = eigenValues[2];
    VECTOR_COPY3(v1,v3);
    eigenValues[2] = t;
    VECTOR_COPY3(v3,vt);
  }

  if ( eigenValues[2] > eigenValues[1] ) {
    t = eigenValues[1];
    VECTOR_COPY3(vt,v2);
    eigenValues[1] = eigenValues[2];
    VECTOR_COPY3(v2,v3);
    eigenValues[2] = t;
    VECTOR_COPY3(v3,vt);
  }

  if (FALSE) {
    /* make sure that my eigen system is right-handed*/
    
    n[0] = v1[1]*v2[2] - v1[2]*v2[1]; 
    n[1] = v1[2]*v2[0] - v1[0]*v2[2]; 
    n[2] = v1[0]*v2[1] - v1[1]*v2[0]; 
    
    if (n[0]*v3[0]+n[1]*v3[1]+n[2]*v3[2] < 0.0 ){
      VECTOR_COPY3(v3,-v3);    
    }
  }
  
  return grid;
}

Grid *gridConvertMetricToJacobian(Grid *grid, double *m, double *j)
{
  int i;
  double eigenValues[3], e1, e2, e3, v1[3], v2[3], v3[3], n[3], vt[3], et;
  if ( grid != gridEigenSystem(grid, m, eigenValues, v1, v2, v3 )) {
    printf("%s: %d: gridConvertMetricToJacobian: gridEigenSystem NULL\n",
	   __FILE__,__LINE__);
    return NULL;
  }

  /* make sure that my eigen system is right-handed*/
    
  n[0] = v1[1]*v2[2] - v1[2]*v2[1]; 
  n[1] = v1[2]*v2[0] - v1[0]*v2[2]; 
  n[2] = v1[0]*v2[1] - v1[1]*v2[0]; 
  
  if (n[0]*v3[0]+n[1]*v3[1]+n[2]*v3[2] < 0.0 ){
    et = eigenValues[1];
    eigenValues[1] = eigenValues[2];
    eigenValues[2] = et;
    VECTOR_COPY3(vt,v2);
    VECTOR_COPY3(v2,v3);
    VECTOR_COPY3(v3,vt);
  }

  e1 = sqrt(eigenValues[0]);
  e2 = sqrt(eigenValues[1]);
  e3 = sqrt(eigenValues[2]);

  /* sqrt(eigenValues) * transpose(eigenVectors) */
  j[0] = e1*v1[0];
  j[1] = e1*v1[1];
  j[2] = e1*v1[2];
  j[3] = e2*v2[0];
  j[4] = e2*v2[1];
  j[5] = e2*v2[2];
  j[6] = e3*v3[0];
  j[7] = e3*v3[1];
  j[8] = e3*v3[2];

  return grid;
}

double gridVolume(Grid *grid, int *nodes )
{
  int ixyz;
  double edge1[3], edge2[3], edge3[3], norm[3], volume; 
  
  for (ixyz = 0 ; ixyz < 3 ; ixyz++ ){
    edge1[ixyz] = grid->xyz[ixyz+3*nodes[1]]
                - grid->xyz[ixyz+3*nodes[0]];
    edge2[ixyz] = grid->xyz[ixyz+3*nodes[2]]
                - grid->xyz[ixyz+3*nodes[0]];
    edge3[ixyz] = grid->xyz[ixyz+3*nodes[3]]
                - grid->xyz[ixyz+3*nodes[0]];
  }

  norm[0] = edge1[1]*edge2[2] - edge1[2]*edge2[1]; 
  norm[1] = edge1[2]*edge2[0] - edge1[0]*edge2[2]; 
  norm[2] = edge1[0]*edge2[1] - edge1[1]*edge2[0]; 

  return  (norm[0]*edge3[0]+norm[1]*edge3[1]+norm[2]*edge3[2])/6.0;
}

Grid *gridNodeAR(Grid *grid, int node, double *ar )
{
  AdjIterator it;
  int cell, nodes[4];
  double local_ar;

  *ar = 1.0;

  for ( it = adjFirst(grid->cellAdj,node); adjValid(it); it = adjNext(it) ){
    cell = adjItem(it);
    gridCell( grid, cell, nodes);
    local_ar = gridAR(grid, nodes);
    if ( local_ar < *ar ) *ar = local_ar;
  }

  return grid;
}

double gridAR(Grid *grid, int *nodes )
{
  double x1, x2, x3, x4; 
  double y1, y2, y3, y4; 
  double z1, z2, z3, z4; 
  double s1, s2, s3, s4, det;
  double xr, yr, zr;
  double circ;
  double nx1, ny1, nz1, rmag1;
  double nx2, ny2, nz2, rmag2;
  double nx3, ny3, nz3, rmag3;
  double nx4, ny4, nz4, rmag4;
  double xins;
  double aspect, cost;
  int i;
  double m[6], m0[6], m1[6], m2[6], m3[6], j[9];

  x1 = grid->xyz[0+3*nodes[0]];
  y1 = grid->xyz[1+3*nodes[0]];
  z1 = grid->xyz[2+3*nodes[0]];

  x2 = grid->xyz[0+3*nodes[1]];
  y2 = grid->xyz[1+3*nodes[1]];
  z2 = grid->xyz[2+3*nodes[1]];

  x3 = grid->xyz[0+3*nodes[2]];
  y3 = grid->xyz[1+3*nodes[2]];
  z3 = grid->xyz[2+3*nodes[2]];

  x4 = grid->xyz[0+3*nodes[3]];
  y4 = grid->xyz[1+3*nodes[3]];
  z4 = grid->xyz[2+3*nodes[3]];

  gridMapMatrix(grid,nodes[0],m0);
  gridMapMatrix(grid,nodes[1],m1);
  gridMapMatrix(grid,nodes[2],m2);
  gridMapMatrix(grid,nodes[3],m3);

  for (i=0;i<6;i++) m[i]=0.25*(m0[i]+m1[i]+m2[i]+m3[i]);
  if (grid != gridConvertMetricToJacobian(grid, m, j) ) {
    printf("%s: %d: gridAR: gridConvertMetricToJacobian NULL\n",
	   __FILE__,__LINE__);
  }
  
  gridMapXYZWithJ(j, &x1, &y1, &z1);
  gridMapXYZWithJ(j, &x2, &y2, &z2);
  gridMapXYZWithJ(j, &x3, &y3, &z3);
  gridMapXYZWithJ(j, &x4, &y4, &z4);

 /* Compute the aspect ratios */

  det = (x4-x1)*((y2-y1)*(z3-z1) - (y3-y1)*(z2-z1))
      + (y4-y1)*((x3-x1)*(z2-z1) - (x2-x1)*(z3-z1))
      + (z4-z1)*((x2-x1)*(y3-y1) - (x3-x1)*(y2-y1));
  s1 = ((x2*x2 + y2*y2 + z2*z2) - (x1*x1 + y1*y1 + z1*z1))/2.0;
  s2 = ((x3*x3 + y3*y3 + z3*z3) - (x1*x1 + y1*y1 + z1*z1))/2.0;
  s3 = ((x4*x4 + y4*y4 + z4*z4) - (x1*x1 + y1*y1 + z1*z1))/2.0;
  xr  =(s3     *((y2-y1)*(z3-z1) - (y3-y1)*(z2-z1))
       	+ (y4-y1)*(s2     *(z2-z1) - s1     *(z3-z1))
	+ (z4-z1)*(s1     *(y3-y1) - s2     *(y2-y1)))/det;
  yr  =((x4-x1)*(s1     *(z3-z1) - s2     *(z2-z1))
	+ s3     *((x3-x1)*(z2-z1) - (x2-x1)*(z3-z1))
	+ (z4-z1)*((x2-x1)*s2      - (x3-x1)*s1     ))/det;
  zr  =((x4-x1)*((y2-y1)*s2      - (y3-y1)*s1     )
	+ (y4-y1)*((x3-x1)*s1      - (x2-x1)*s2     )
	+ s3     *((x2-x1)*(y3-y1) - (x3-x1)*(y2-y1)))/det;
  circ = sqrt((x1-xr)*(x1-xr) + (y1-yr)*(y1-yr) + (z1-zr)*(z1-zr));

  /* Get the in-circle */

  nx1 = (y3 - y1)*(z2 - z1) - (y2 - y1)*(z3 - z1);
  ny1 =-(x3 - x1)*(z2 - z1) + (x2 - x1)*(z3 - z1);
  nz1 = (x3 - x1)*(y2 - y1) - (x2 - x1)*(y3 - y1);
  rmag1 = sqrt(nx1*nx1 + ny1*ny1 + nz1*nz1);

  nx2 = (y2 - y1)*(z4 - z1) - (y4 - y1)*(z2 - z1);
  ny2 =-(x2 - x1)*(z4 - z1) + (x4 - x1)*(z2 - z1);
  nz2 = (x2 - x1)*(y4 - y1) - (x4 - x1)*(y2 - y1);
  rmag2 = sqrt(nx2*nx2 + ny2*ny2 + nz2*nz2);

  nx3 = (y4 - y1)*(z3 - z1) - (y3 - y1)*(z4 - z1);
  ny3 =-(x4 - x1)*(z3 - z1) + (x3 - x1)*(z4 - z1);
  nz3 = (x4 - x1)*(y3 - y1) - (x3 - x1)*(y4 - y1);
  rmag3 = sqrt(nx3*nx3 + ny3*ny3 + nz3*nz3);

  nx4 = (y3 - y2)*(z4 - z2) - (y4 - y2)*(z3 - z2);
  ny4 =-(x3 - x2)*(z4 - z2) + (x4 - x2)*(z3 - z2);
  nz4 = (x3 - x2)*(y4 - y2) - (x4 - x2)*(y3 - y2);
  rmag4 = sqrt(nx4*nx4 + ny4*ny4 + nz4*nz4);
  nx1 = nx1/rmag1;
  ny1 = ny1/rmag1;
  nz1 = nz1/rmag1;
  nx2 = nx2/rmag2;
  ny2 = ny2/rmag2;
  nz2 = nz2/rmag2;
  nx3 = nx3/rmag3;
  ny3 = ny3/rmag3;
  nz3 = nz3/rmag3;
  nx4 = nx4/rmag4;
  ny4 = ny4/rmag4;
  nz4 = nz4/rmag4;
  det= -(nx3*ny2*nz1) + nx4*ny2*nz1 + nx2*ny3*nz1 - nx4*ny3*nz1
    -nx2*ny4*nz1 + nx3*ny4*nz1 + nx3*ny1*nz2 - nx4*ny1*nz2
    -nx1*ny3*nz2 + nx4*ny3*nz2 + nx1*ny4*nz2 - nx3*ny4*nz2
    -nx2*ny1*nz3 + nx4*ny1*nz3 + nx1*ny2*nz3 - nx4*ny2*nz3
    -nx1*ny4*nz3 + nx2*ny4*nz3 + nx2*ny1*nz4 - nx3*ny1*nz4
    -nx1*ny2*nz4 + nx3*ny2*nz4 + nx1*ny3*nz4 - nx2*ny3*nz4;
  s1 = nx1*x1 + ny1*y1 + nz1*z1;
  s2 = nx2*x1 + ny2*y1 + nz2*z1;
  s3 = nx3*x1 + ny3*y1 + nz3*z1;
  s4 = nx4*x4 + ny4*y4 + nz4*z4;
  xins = (nx4*ny3*nz2*s1 - nx3*ny4*nz2*s1 - nx4*ny2*nz3*s1 +
	  nx2*ny4*nz3*s1 +
	  nx3*ny2*nz4*s1 - nx2*ny3*nz4*s1 - nx4*ny3*nz1*s2 +
	  nx3*ny4*nz1*s2 +
	  nx4*ny1*nz3*s2 - nx1*ny4*nz3*s2 - nx3*ny1*nz4*s2 +
	  nx1*ny3*nz4*s2 +
	  nx4*ny2*nz1*s3 - nx2*ny4*nz1*s3 - nx4*ny1*nz2*s3 +
	  nx1*ny4*nz2*s3 +
	  nx2*ny1*nz4*s3 - nx1*ny2*nz4*s3 - nx3*ny2*nz1*s4 +
	  nx2*ny3*nz1*s4 +
	  nx3*ny1*nz2*s4 - nx1*ny3*nz2*s4 - nx2*ny1*nz3*s4 +
	  nx1*ny2*nz3*s4)/det;

  aspect = xins/circ*3.0;

  if ( aspect < 0.0 ) {
    printf("nodes %d %d %d %d aspect %f\n",nodes[0],nodes[1],nodes[2],nodes[3],aspect);
    printf("m \n %25.15f %25.15f %25.15f \n %25.15f %25.15f %25.15f \n %25.15f %25.15f %25.15f\n",m[0],m[1],m[2],m[1],m[3],m[4],m[2],m[4],m[5]);
    printf("j \n %25.15f %25.15f %25.15f \n %25.15f %25.15f %25.15f \n %25.15f %25.15f %25.15f\n",j[0],j[1],j[2],j[1],j[3],j[4],j[2],j[4],j[5]);
  }

  if ( gridVolume( grid, nodes ) <= 1.0e-14) aspect = -1.0;
  return aspect;
}


Grid *gridNodeARDerivative (Grid *grid, int node, double *ar, double *dARdx )
{
  AdjIterator it;
  int cell, nodes[4];
  double local_ar, local_dARdx[3];

  *ar = 1.1;
  dARdx[0] = DBL_MAX;
  dARdx[1] = DBL_MAX;
  dARdx[2] = DBL_MAX;

  for ( it = adjFirst(grid->cellAdj,node); adjValid(it); it = adjNext(it) ){
    cell = adjItem(it);
    nodes[0] = node;
    if (node == grid->c2n[0+4*cell]){
      nodes[1] = grid->c2n[1+4*cell];
    }else{
      nodes[1] = grid->c2n[0+4*cell];
    }
    gridOrient( grid, &grid->c2n[4*cell], nodes);
    if ( grid != gridCellARDerivative(grid, nodes, &local_ar, local_dARdx ) ) {
      *ar = 0.0;
      dARdx[0] = DBL_MAX;
      dARdx[1] = DBL_MAX;
      dARdx[2] = DBL_MAX;
      return NULL;
    }
    if ( local_ar < *ar ) {
      *ar = local_ar;
      dARdx[0] = local_dARdx[0];
      dARdx[1] = local_dARdx[1];
      dARdx[2] = local_dARdx[2];
    }
  }

  return grid;
}

Grid *gridCellARDerivative(Grid *grid, int *nodes, double *ar, double *dARdx )
{

  double x1, x2, x3, x4; 
  double y1, y2, y3, y4; 
  double z1, z2, z3, z4; 
  double s1, s2, s3, s4, det;
  double xr, yr, zr;
  double circ;
  double nx1, ny1, nz1, rmag1;
  double nx2, ny2, nz2, rmag2;
  double nx3, ny3, nz3, rmag3;
  double nx4, ny4, nz4, rmag4;
  double xins;

  double s1_dx, s2_dx, s3_dx, s4_dx, det_dx;
  double s1_dy, s2_dy, s3_dy, s4_dy, det_dy;
  double s1_dz, s2_dz, s3_dz, s4_dz, det_dz;
  double xr_dx, yr_dx, zr_dx;
  double xr_dy, yr_dy, zr_dy;
  double xr_dz, yr_dz, zr_dz;
  double circ_dx, circ_dy, circ_dz; 

  double nx1_dx, ny1_dx, nz1_dx, rmag1_dx;
  double nx1_dy, ny1_dy, nz1_dy, rmag1_dy;
  double nx1_dz, ny1_dz, nz1_dz, rmag1_dz;

  double nx2_dx, ny2_dx, nz2_dx, rmag2_dx;
  double nx2_dy, ny2_dy, nz2_dy, rmag2_dy;
  double nx2_dz, ny2_dz, nz2_dz, rmag2_dz;

  double nx3_dx, ny3_dx, nz3_dx, rmag3_dx;
  double nx3_dy, ny3_dy, nz3_dy, rmag3_dy;
  double nx3_dz, ny3_dz, nz3_dz, rmag3_dz;

  double nx4_dx, ny4_dx, nz4_dx;
  double nx4_dy, ny4_dy, nz4_dy;
  double nx4_dz, ny4_dz, nz4_dz;

  double xins_dx, xins_dy, xins_dz;

  int i;
  double m[6], m0[6], m1[6], m2[6], m3[6], j[9];

  x1 = grid->xyz[0+3*nodes[0]];
  y1 = grid->xyz[1+3*nodes[0]];
  z1 = grid->xyz[2+3*nodes[0]];

  x2 = grid->xyz[0+3*nodes[1]];
  y2 = grid->xyz[1+3*nodes[1]];
  z2 = grid->xyz[2+3*nodes[1]];

  x3 = grid->xyz[0+3*nodes[2]];
  y3 = grid->xyz[1+3*nodes[2]];
  z3 = grid->xyz[2+3*nodes[2]];

  x4 = grid->xyz[0+3*nodes[3]];
  y4 = grid->xyz[1+3*nodes[3]];
  z4 = grid->xyz[2+3*nodes[3]];

  gridMapMatrix(grid,nodes[0],m0);
  gridMapMatrix(grid,nodes[1],m1);
  gridMapMatrix(grid,nodes[2],m2);
  gridMapMatrix(grid,nodes[3],m3);

  for (i=0;i<6;i++) m[i]=0.25*(m0[i]+m1[i]+m2[i]+m3[i]);
  if (grid != gridConvertMetricToJacobian(grid, m, j) ) {
    printf("%s: %d: gridCellARDerivative: gridConvertMetricToJacobian NULL\n",
	   __FILE__,__LINE__);
  }

  gridMapXYZWithJ(j, &x1, &y1, &z1);
  gridMapXYZWithJ(j, &x2, &y2, &z2);
  gridMapXYZWithJ(j, &x3, &y3, &z3);
  gridMapXYZWithJ(j, &x4, &y4, &z4);

  /* Compute the aspect ratios */

        det = (x4-x1)*((y2-y1)*(z3-z1) - (y3-y1)*(z2-z1))
            + (y4-y1)*((x3-x1)*(z2-z1) - (x2-x1)*(z3-z1))
            + (z4-z1)*((x2-x1)*(y3-y1) - (x3-x1)*(y2-y1));

	det_dx = -((y2-y1)*(z3-z1) - (y3-y1)*(z2-z1))
   	       + (y4-y1)*(-(z2-z1) + (z3-z1))
	       + (z4-z1)*(-(y3-y1) + (y2-y1));

        det_dy = (x4-x1)*(-(z3-z1) +(z2-z1))
            - ((x3-x1)*(z2-z1) - (x2-x1)*(z3-z1))
            + (z4-z1)*(-(x2-x1) + (x3-x1));

        det_dz = (x4-x1)*(-(y2-y1) + (y3-y1))
            + (y4-y1)*(-(x3-x1) + (x2-x1))
            -((x2-x1)*(y3-y1) - (x3-x1)*(y2-y1));

         s1 = ((x2*x2 + y2*y2 + z2*z2) - (x1*x1 + y1*y1 + z1*z1))/2.0;
         s2 = ((x3*x3 + y3*y3 + z3*z3) - (x1*x1 + y1*y1 + z1*z1))/2.0;
         s3 = ((x4*x4 + y4*y4 + z4*z4) - (x1*x1 + y1*y1 + z1*z1))/2.0;

	 s1_dx = -x1;
	 s1_dy = -y1;
	 s1_dz = -z1;
	 s2_dx = -x1;
	 s2_dy = -y1;
	 s2_dz = -z1;
	 s3_dx = -x1;
	 s3_dy = -y1;
	 s3_dz = -z1;


         xr  = (   s3    * ( (y2-y1)*(z3-z1) - (y3-y1)*(z2-z1))
               + (y4-y1) * (    s2  *(z2-z1) -   s1   *(z3-z1))
               + (z4-z1) * (    s1  *(y3-y1) -   s2   *(y2-y1)) ) / det;


	 xr_dx = (  s3_dx  * ( (y2-y1)*(z3-z1) - (y3-y1)*(z2-z1))
                 + (y4-y1) * (  s2_dx *(z2-z1) -  s1_dx *(z3-z1))
                 + (z4-z1) * (  s1_dx *(y3-y1) -  s2_dx *(y2-y1)) ) ;

	 xr_dx = det * xr_dx - (xr*det) * det_dx ;
	 xr_dx = xr_dx / det / det;

	 xr_dy = (   s3    * (        -(z3-z1) +         (z2-z1))
                 +  s3_dy  * ( (y2-y1)*(z3-z1) - (y3-y1)*(z2-z1))
                 + (y4-y1) * (  s2_dy *(z2-z1) -  s1_dy *(z3-z1))
                 -           (    s2  *(z2-z1) -   s1   *(z3-z1))
                 + (z4-z1) * ( s1_dy  *(y3-y1) -  s2_dy *(y2-y1)) 
                 + (z4-z1) * (        -s1      +        s2     ) ) ;

	 xr_dy = det * xr_dy - (xr*det) * det_dy ;
	 xr_dy = xr_dy / det / det;


         xr_dz = (   s3    * (           -(y2-y1) +            (y3-y1))
	         +  s3_dz  * (    (y2-y1)*(z3-z1) -    (y3-y1)*(z2-z1))
                 + (y4-y1) * (    -s2             +    s1             )
                 + (y4-y1) * (    s2_dz  *(z2-z1) -   s1_dz   *(z3-z1))
                 + (z4-z1) * (    s1_dz  *(y3-y1) -   s2_dz   *(y2-y1)) 
                 -           (    s1     *(y3-y1) -   s2      *(y2-y1)) );
	 xr_dz = det * xr_dz - (xr*det) * det_dz ;
	 xr_dz = xr_dz / det / det;

         yr  =((x4-x1)*(s1     *(z3-z1) - s2     *(z2-z1))
             + s3     *((x3-x1)*(z2-z1) - (x2-x1)*(z3-z1))
             + (z4-z1)*((x2-x1)*s2      - (x3-x1)*s1     ))/det;

         yr_dx = ( (x4-x1)*(s1_dx  *(z3-z1) - s2_dx  *(z2-z1))
		 -         (s1     *(z3-z1) - s2     *(z2-z1))
                 + s3     *(       -(z2-z1)          +(z3-z1))
                 + s3_dx  *((x3-x1)*(z2-z1) - (x2-x1)*(z3-z1))
                 + (z4-z1)*(       -s2               +s1     )
                 + (z4-z1)*((x2-x1)*s2_dx   - (x3-x1)*s1_dx  ));

	 yr_dx = det * yr_dx - (yr*det) * det_dx ;
	 yr_dx = yr_dx / det / det;

         yr_dy  = ( (x4-x1)*(s1_dy  *(z3-z1) - s2_dy  *(z2-z1))
                  +  s3_dy *((x3-x1)*(z2-z1) - (x2-x1)*(z3-z1))
                  + (z4-z1)*((x2-x1)*s2_dy   - (x3-x1)*s1_dy  ) );

	 yr_dy = det * yr_dy - (yr*det) * det_dy ;
	 yr_dy = yr_dy / det / det;


         yr_dz = ( (x4-x1)*(  -s1           + s2             )
		 + (x4-x1)*( s1_dz *(z3-z1) - s2_dz  *(z2-z1))
                 + s3     *(-(x3-x1)        + (x2-x1)        )
                 + s3_dz  *((x3-x1)*(z2-z1) - (x2-x1)*(z3-z1))
                 + (z4-z1)*((x2-x1)*s2_dz   - (x3-x1)*s1_dz  )
                 -         ((x2-x1)*s2      - (x3-x1)*s1     ) );

	 yr_dz = det * yr_dz - (yr*det) * det_dz ;
	 yr_dz = yr_dz / det / det;

         zr  =((x4-x1)*((y2-y1)*s2      - (y3-y1)*s1     )
             + (y4-y1)*((x3-x1)*s1      - (x2-x1)*s2     )
             + s3     *((x2-x1)*(y3-y1) - (x3-x1)*(y2-y1)))/det;

         zr_dx = ( (x4-x1)*((y2-y1)*s2_dx   - (y3-y1)*s1_dx  )
		 -         ((y2-y1)*s2      - (y3-y1)*s1     )
                 + (y4-y1)*(       -s1      +         s2     )
                 + (y4-y1)*((x3-x1)*s1_dx   - (x2-x1)*s2_dx  )
                 + s3     *(       -(y3-y1) +         (y2-y1))
                 + s3_dx  *((x2-x1)*(y3-y1) - (x3-x1)*(y2-y1)) );

	 zr_dx = det * zr_dx - (zr*det) * det_dx ;
	 zr_dx = zr_dx / det / det;

         zr_dy = ( (x4-x1)*((y2-y1)*s2_dy   - (y3-y1)*s1_dy  )
		 + (x4-x1)*(       -s2      +         s1     )
                 + (y4-y1)*((x3-x1)*s1_dy   - (x2-x1)*s2_dy  )
                 -         ((x3-x1)*s1      - (x2-x1)*s2     )
                 + s3     *(-(x2-x1)        + (x3-x1)        )
                 + s3_dy  *((x2-x1)*(y3-y1) - (x3-x1)*(y2-y1)) );

	 zr_dy = det * zr_dy - (zr*det) * det_dy ;
	 zr_dy = zr_dy / det / det;

         zr_dz = ( (x4-x1)*((y2-y1)*s2_dz   - (y3-y1)*s1_dz  )
                 + (y4-y1)*((x3-x1)*s1_dz   - (x2-x1)*s2_dz  )
                 + s3_dz  *((x2-x1)*(y3-y1) - (x3-x1)*(y2-y1)) );

	 zr_dz = det * zr_dz - (zr*det) * det_dz ;
	 zr_dz = zr_dz / det / det;

         circ = sqrt((x1-xr)*(x1-xr) + (y1-yr)*(y1-yr) + (z1-zr)*(z1-zr));

	 circ_dx = 2.0*(x1-xr)*(1.0-xr_dx) 
	         + 2.0*(y1-yr)*(   -yr_dx) 
	         + 2.0*(z1-zr)*(   -zr_dx);

	 circ_dx = 0.5 / circ * circ_dx;

	 circ_dy = 2.0*(x1-xr)*(   -xr_dy) 
	         + 2.0*(y1-yr)*(1.0-yr_dy) 
	         + 2.0*(z1-zr)*(   -zr_dy);

	 circ_dy = 0.5 / circ * circ_dy;

	 circ_dz = 2.0*(x1-xr)*(   -xr_dz) 
	         + 2.0*(y1-yr)*(   -yr_dz) 
	         + 2.0*(z1-zr)*(1.0-zr_dz);

	 circ_dz = 0.5 / circ * circ_dz;
 
  /* Get the in-circle */

         nx1 = (y3 - y1)*(z2 - z1) - (y2 - y1)*(z3 - z1);
         ny1 =-(x3 - x1)*(z2 - z1) + (x2 - x1)*(z3 - z1);
         nz1 = (x3 - x1)*(y2 - y1) - (x2 - x1)*(y3 - y1);

	 nx1_dx = 0.0;
	 ny1_dx = (z2 - z1) - (z3 - z1);
         nz1_dx =-(y2 - y1) + (y3 - y1);

	 nx1_dy = -(z2 - z1) + (z3 - z1);
	 ny1_dy = 0.0;
         nz1_dy = -(x3 - x1) + (x2 - x1);
 
	 nx1_dz = -(y3 - y1) + (y2 - y1);
	 ny1_dz =  (x3 - x1) - (x2 - x1);
         nz1_dz = 0.0;

         nx2 = (y2 - y1)*(z4 - z1) - (y4 - y1)*(z2 - z1);
         ny2 =-(x2 - x1)*(z4 - z1) + (x4 - x1)*(z2 - z1);
         nz2 = (x2 - x1)*(y4 - y1) - (x4 - x1)*(y2 - y1);

         nx2_dx =  0.0;
         ny2_dx =  (z4 - z1) - (z2 - z1);
         nz2_dx = -(y4 - y1) + (y2 - y1);

         nx2_dy = -(z4 - z1) + (z2 - z1);
         ny2_dy =  0.0;
         nz2_dy = -(x2 - x1) + (x4 - x1);

         nx2_dz = -(y2 - y1) + (y4 - y1);
         ny2_dz =  (x2 - x1) - (x4 - x1);
         nz2_dz =  0.0;

         nx3 = (y4 - y1)*(z3 - z1) - (y3 - y1)*(z4 - z1);
         ny3 =-(x4 - x1)*(z3 - z1) + (x3 - x1)*(z4 - z1);
         nz3 = (x4 - x1)*(y3 - y1) - (x3 - x1)*(y4 - y1);

         nx3_dx =  0.0;
         ny3_dx =  (z3 - z1) - (z4 - z1);
         nz3_dx = -(y3 - y1) + (y4 - y1);

         nx3_dy = -(z3 - z1) + (z4 - z1);
         ny3_dy = 0.0;
         nz3_dy = -(x4 - x1) + (x3 - x1);

         nx3_dz =-(y4 - y1) + (y3 - y1);
         ny3_dz = (x4 - x1) - (x3 - x1);
         nz3_dz = 0.0;

         nx4 = (y3 - y2)*(z4 - z2) - (y4 - y2)*(z3 - z2);
         ny4 =-(x3 - x2)*(z4 - z2) + (x4 - x2)*(z3 - z2);
         nz4 = (x3 - x2)*(y4 - y2) - (x4 - x2)*(y3 - y2);

         rmag1 = sqrt(nx1*nx1 + ny1*ny1 + nz1*nz1);

	 rmag1_dx = (nx1*nx1_dx + ny1*ny1_dx + nz1*nz1_dx) / rmag1;
	 rmag1_dy = (nx1*nx1_dy + ny1*ny1_dy + nz1*nz1_dy) / rmag1;
	 rmag1_dz = (nx1*nx1_dz + ny1*ny1_dz + nz1*nz1_dz) / rmag1;

         rmag2 = sqrt(nx2*nx2 + ny2*ny2 + nz2*nz2);

	 rmag2_dx = (nx2*nx2_dx + ny2*ny2_dx + nz2*nz2_dx) / rmag2;
	 rmag2_dy = (nx2*nx2_dy + ny2*ny2_dy + nz2*nz2_dy) / rmag2;
	 rmag2_dz = (nx2*nx2_dz + ny2*ny2_dz + nz2*nz2_dz) / rmag2;

         rmag3 = sqrt(nx3*nx3 + ny3*ny3 + nz3*nz3);

	 rmag3_dx = (nx3*nx3_dx + ny3*ny3_dx + nz3*nz3_dx) / rmag3;
	 rmag3_dy = (nx3*nx3_dy + ny3*ny3_dy + nz3*nz3_dy) / rmag3;
	 rmag3_dz = (nx3*nx3_dz + ny3*ny3_dz + nz3*nz3_dz) / rmag3;

         rmag4 = sqrt(nx4*nx4 + ny4*ny4 + nz4*nz4);
	
	 /* note that the derivatives procede the normalization */
	 /* because I need the un-nornmalized value for deriavtives */
 
         nx1_dx = ( rmag1 * nx1_dx - nx1 * rmag1_dx ) / rmag1 / rmag1;
         ny1_dx = ( rmag1 * ny1_dx - ny1 * rmag1_dx ) / rmag1 / rmag1;
         nz1_dx = ( rmag1 * nz1_dx - nz1 * rmag1_dx ) / rmag1 / rmag1;

         nx1_dy = ( rmag1 * nx1_dy - nx1 * rmag1_dy ) / rmag1 / rmag1;
         ny1_dy = ( rmag1 * ny1_dy - ny1 * rmag1_dy ) / rmag1 / rmag1;
         nz1_dy = ( rmag1 * nz1_dy - nz1 * rmag1_dy ) / rmag1 / rmag1;

         nx1_dz = ( rmag1 * nx1_dz - nx1 * rmag1_dz ) / rmag1 / rmag1;
         ny1_dz = ( rmag1 * ny1_dz - ny1 * rmag1_dz ) / rmag1 / rmag1;
         nz1_dz = ( rmag1 * nz1_dz - nz1 * rmag1_dz ) / rmag1 / rmag1;

         nx1 = nx1/rmag1;
         ny1 = ny1/rmag1;
         nz1 = nz1/rmag1;

         nx2_dx = ( rmag2 * nx2_dx - nx2 * rmag2_dx ) / rmag2 / rmag2;
         ny2_dx = ( rmag2 * ny2_dx - ny2 * rmag2_dx ) / rmag2 / rmag2;
         nz2_dx = ( rmag2 * nz2_dx - nz2 * rmag2_dx ) / rmag2 / rmag2;

         nx2_dy = ( rmag2 * nx2_dy - nx2 * rmag2_dy ) / rmag2 / rmag2;
         ny2_dy = ( rmag2 * ny2_dy - ny2 * rmag2_dy ) / rmag2 / rmag2;
         nz2_dy = ( rmag2 * nz2_dy - nz2 * rmag2_dy ) / rmag2 / rmag2;

         nx2_dz = ( rmag2 * nx2_dz - nx2 * rmag2_dz ) / rmag2 / rmag2;
         ny2_dz = ( rmag2 * ny2_dz - ny2 * rmag2_dz ) / rmag2 / rmag2;
         nz2_dz = ( rmag2 * nz2_dz - nz2 * rmag2_dz ) / rmag2 / rmag2;

         nx2 = nx2/rmag2;
         ny2 = ny2/rmag2;
         nz2 = nz2/rmag2;

         nx3_dx = ( rmag3 * nx3_dx - nx3 * rmag3_dx ) / rmag3 / rmag3;
         ny3_dx = ( rmag3 * ny3_dx - ny3 * rmag3_dx ) / rmag3 / rmag3;
         nz3_dx = ( rmag3 * nz3_dx - nz3 * rmag3_dx ) / rmag3 / rmag3;

         nx3_dy = ( rmag3 * nx3_dy - nx3 * rmag3_dy ) / rmag3 / rmag3;
         ny3_dy = ( rmag3 * ny3_dy - ny3 * rmag3_dy ) / rmag3 / rmag3;
         nz3_dy = ( rmag3 * nz3_dy - nz3 * rmag3_dy ) / rmag3 / rmag3;

         nx3_dz = ( rmag3 * nx3_dz - nx3 * rmag3_dz ) / rmag3 / rmag3;
         ny3_dz = ( rmag3 * ny3_dz - ny3 * rmag3_dz ) / rmag3 / rmag3;
         nz3_dz = ( rmag3 * nz3_dz - nz3 * rmag3_dz ) / rmag3 / rmag3;

         nx3 = nx3/rmag3;
         ny3 = ny3/rmag3;
         nz3 = nz3/rmag3;

         nx4_dx = 0.0;
         ny4_dx = 0.0;
         nz4_dx = 0.0;

         nx4_dy = 0.0;
         ny4_dy = 0.0;
         nz4_dy = 0.0;

         nx4_dz = 0.0;
         ny4_dz = 0.0;
         nz4_dz = 0.0;

         nx4 = nx4/rmag4;
         ny4 = ny4/rmag4;
         nz4 = nz4/rmag4;

         det= -nx3*ny2*nz1 + nx4*ny2*nz1 + nx2*ny3*nz1 - nx4*ny3*nz1
              -nx2*ny4*nz1 + nx3*ny4*nz1 + nx3*ny1*nz2 - nx4*ny1*nz2
              -nx1*ny3*nz2 + nx4*ny3*nz2 + nx1*ny4*nz2 - nx3*ny4*nz2
              -nx2*ny1*nz3 + nx4*ny1*nz3 + nx1*ny2*nz3 - nx4*ny2*nz3
              -nx1*ny4*nz3 + nx2*ny4*nz3 + nx2*ny1*nz4 - nx3*ny1*nz4
              -nx1*ny2*nz4 + nx3*ny2*nz4 + nx1*ny3*nz4 - nx2*ny3*nz4;

         det_dx = 
	   - nx3_dx*ny2*nz1 - nx3*ny2_dx*nz1 - nx3*ny2*nz1_dx 
	   + nx4_dx*ny2*nz1 + nx4*ny2_dx*nz1 + nx4*ny2*nz1_dx 
	   + nx2_dx*ny3*nz1 + nx2*ny3_dx*nz1 + nx2*ny3*nz1_dx 
	   - nx4_dx*ny3*nz1 - nx4*ny3_dx*nz1 - nx4*ny3*nz1_dx
	   - nx2_dx*ny4*nz1 - nx2*ny4_dx*nz1 - nx2*ny4*nz1_dx 
	   + nx3_dx*ny4*nz1 + nx3*ny4_dx*nz1 + nx3*ny4*nz1_dx
	   + nx3_dx*ny1*nz2 + nx3*ny1_dx*nz2 + nx3*ny1*nz2_dx 
	   - nx4_dx*ny1*nz2 - nx4*ny1_dx*nz2 - nx4*ny1*nz2_dx
	   - nx1_dx*ny3*nz2 - nx1*ny3_dx*nz2 - nx1*ny3*nz2_dx 
	   + nx4_dx*ny3*nz2 + nx4*ny3_dx*nz2 + nx4*ny3*nz2_dx 
	   + nx1_dx*ny4*nz2 + nx1*ny4_dx*nz2 + nx1*ny4*nz2_dx 
	   - nx3_dx*ny4*nz2 - nx3*ny4_dx*nz2 - nx3*ny4*nz2_dx
	   - nx2_dx*ny1*nz3 - nx2*ny1_dx*nz3 - nx2*ny1*nz3_dx 
	   + nx4_dx*ny1*nz3 + nx4*ny1_dx*nz3 + nx4*ny1*nz3_dx 
	   + nx1_dx*ny2*nz3 + nx1*ny2_dx*nz3 + nx1*ny2*nz3_dx 
	   - nx4_dx*ny2*nz3 - nx4*ny2_dx*nz3 - nx4*ny2*nz3_dx
	   - nx1_dx*ny4*nz3 - nx1*ny4_dx*nz3 - nx1*ny4*nz3_dx 
	   + nx2_dx*ny4*nz3 + nx2*ny4_dx*nz3 + nx2*ny4*nz3_dx 
	   + nx2_dx*ny1*nz4 + nx2*ny1_dx*nz4 + nx2*ny1*nz4_dx 
	   - nx3_dx*ny1*nz4 - nx3*ny1_dx*nz4 - nx3*ny1*nz4_dx
	   - nx1_dx*ny2*nz4 - nx1*ny2_dx*nz4 - nx1*ny2*nz4_dx 
	   + nx3_dx*ny2*nz4 + nx3*ny2_dx*nz4 + nx3*ny2*nz4_dx 
	   + nx1_dx*ny3*nz4 + nx1*ny3_dx*nz4 + nx1*ny3*nz4_dx 
	   - nx2_dx*ny3*nz4 - nx2*ny3_dx*nz4 - nx2*ny3*nz4_dx;

         det_dy = 
	   - nx3_dy*ny2*nz1 - nx3*ny2_dy*nz1 - nx3*ny2*nz1_dy 
	   + nx4_dy*ny2*nz1 + nx4*ny2_dy*nz1 + nx4*ny2*nz1_dy 
	   + nx2_dy*ny3*nz1 + nx2*ny3_dy*nz1 + nx2*ny3*nz1_dy 
	   - nx4_dy*ny3*nz1 - nx4*ny3_dy*nz1 - nx4*ny3*nz1_dy
	   - nx2_dy*ny4*nz1 - nx2*ny4_dy*nz1 - nx2*ny4*nz1_dy 
	   + nx3_dy*ny4*nz1 + nx3*ny4_dy*nz1 + nx3*ny4*nz1_dy
	   + nx3_dy*ny1*nz2 + nx3*ny1_dy*nz2 + nx3*ny1*nz2_dy 
	   - nx4_dy*ny1*nz2 - nx4*ny1_dy*nz2 - nx4*ny1*nz2_dy
	   - nx1_dy*ny3*nz2 - nx1*ny3_dy*nz2 - nx1*ny3*nz2_dy 
	   + nx4_dy*ny3*nz2 + nx4*ny3_dy*nz2 + nx4*ny3*nz2_dy 
	   + nx1_dy*ny4*nz2 + nx1*ny4_dy*nz2 + nx1*ny4*nz2_dy 
	   - nx3_dy*ny4*nz2 - nx3*ny4_dy*nz2 - nx3*ny4*nz2_dy
	   - nx2_dy*ny1*nz3 - nx2*ny1_dy*nz3 - nx2*ny1*nz3_dy 
	   + nx4_dy*ny1*nz3 + nx4*ny1_dy*nz3 + nx4*ny1*nz3_dy 
	   + nx1_dy*ny2*nz3 + nx1*ny2_dy*nz3 + nx1*ny2*nz3_dy 
	   - nx4_dy*ny2*nz3 - nx4*ny2_dy*nz3 - nx4*ny2*nz3_dy
	   - nx1_dy*ny4*nz3 - nx1*ny4_dy*nz3 - nx1*ny4*nz3_dy 
	   + nx2_dy*ny4*nz3 + nx2*ny4_dy*nz3 + nx2*ny4*nz3_dy 
	   + nx2_dy*ny1*nz4 + nx2*ny1_dy*nz4 + nx2*ny1*nz4_dy 
	   - nx3_dy*ny1*nz4 - nx3*ny1_dy*nz4 - nx3*ny1*nz4_dy
	   - nx1_dy*ny2*nz4 - nx1*ny2_dy*nz4 - nx1*ny2*nz4_dy 
	   + nx3_dy*ny2*nz4 + nx3*ny2_dy*nz4 + nx3*ny2*nz4_dy 
	   + nx1_dy*ny3*nz4 + nx1*ny3_dy*nz4 + nx1*ny3*nz4_dy 
	   - nx2_dy*ny3*nz4 - nx2*ny3_dy*nz4 - nx2*ny3*nz4_dy;

         det_dz = 
	   - nx3_dz*ny2*nz1 - nx3*ny2_dz*nz1 - nx3*ny2*nz1_dz 
	   + nx4_dz*ny2*nz1 + nx4*ny2_dz*nz1 + nx4*ny2*nz1_dz 
	   + nx2_dz*ny3*nz1 + nx2*ny3_dz*nz1 + nx2*ny3*nz1_dz 
	   - nx4_dz*ny3*nz1 - nx4*ny3_dz*nz1 - nx4*ny3*nz1_dz
	   - nx2_dz*ny4*nz1 - nx2*ny4_dz*nz1 - nx2*ny4*nz1_dz 
	   + nx3_dz*ny4*nz1 + nx3*ny4_dz*nz1 + nx3*ny4*nz1_dz
	   + nx3_dz*ny1*nz2 + nx3*ny1_dz*nz2 + nx3*ny1*nz2_dz 
	   - nx4_dz*ny1*nz2 - nx4*ny1_dz*nz2 - nx4*ny1*nz2_dz
	   - nx1_dz*ny3*nz2 - nx1*ny3_dz*nz2 - nx1*ny3*nz2_dz 
	   + nx4_dz*ny3*nz2 + nx4*ny3_dz*nz2 + nx4*ny3*nz2_dz 
	   + nx1_dz*ny4*nz2 + nx1*ny4_dz*nz2 + nx1*ny4*nz2_dz 
	   - nx3_dz*ny4*nz2 - nx3*ny4_dz*nz2 - nx3*ny4*nz2_dz
	   - nx2_dz*ny1*nz3 - nx2*ny1_dz*nz3 - nx2*ny1*nz3_dz 
	   + nx4_dz*ny1*nz3 + nx4*ny1_dz*nz3 + nx4*ny1*nz3_dz 
	   + nx1_dz*ny2*nz3 + nx1*ny2_dz*nz3 + nx1*ny2*nz3_dz 
	   - nx4_dz*ny2*nz3 - nx4*ny2_dz*nz3 - nx4*ny2*nz3_dz
	   - nx1_dz*ny4*nz3 - nx1*ny4_dz*nz3 - nx1*ny4*nz3_dz 
	   + nx2_dz*ny4*nz3 + nx2*ny4_dz*nz3 + nx2*ny4*nz3_dz 
	   + nx2_dz*ny1*nz4 + nx2*ny1_dz*nz4 + nx2*ny1*nz4_dz 
	   - nx3_dz*ny1*nz4 - nx3*ny1_dz*nz4 - nx3*ny1*nz4_dz
	   - nx1_dz*ny2*nz4 - nx1*ny2_dz*nz4 - nx1*ny2*nz4_dz 
	   + nx3_dz*ny2*nz4 + nx3*ny2_dz*nz4 + nx3*ny2*nz4_dz 
	   + nx1_dz*ny3*nz4 + nx1*ny3_dz*nz4 + nx1*ny3*nz4_dz 
	   - nx2_dz*ny3*nz4 - nx2*ny3_dz*nz4 - nx2*ny3*nz4_dz;

         s1 = nx1*x1 + ny1*y1 + nz1*z1;

         s1_dx = nx1_dx*x1 + nx1 + ny1_dx*y1       + nz1_dx*z1;
         s1_dy = nx1_dy*x1       + ny1_dy*y1 + ny1 + nz1_dy*z1;
         s1_dz = nx1_dz*x1       + ny1_dz*y1       + nz1_dz*z1 + nz1;

         s2 = nx2*x1 + ny2*y1 + nz2*z1;

         s2_dx = nx2_dx*x1 + nx2 + ny2_dx*y1       + nz2_dx*z1;
         s2_dy = nx2_dy*x1       + ny2_dy*y1 + ny2 + nz2_dy*z1;
         s2_dz = nx2_dz*x1       + ny2_dz*y1       + nz2_dz*z1 + nz2;

         s3 = nx3*x1 + ny3*y1 + nz3*z1;

         s3_dx = nx3_dx*x1 + nx3 + ny3_dx*y1       + nz3_dx*z1;
         s3_dy = nx3_dy*x1       + ny3_dy*y1 + ny3 + nz3_dy*z1;
         s3_dz = nx3_dz*x1       + ny3_dz*y1       + nz3_dz*z1 + nz3;

         s4 = nx4*x4 + ny4*y4 + nz4*z4;

         s4_dx = 0.0;
         s4_dy = 0.0;
         s4_dz = 0.0;

         xins = nx4*ny3*nz2*s1 - nx3*ny4*nz2*s1 - nx4*ny2*nz3*s1 +
                nx2*ny4*nz3*s1 +
                nx3*ny2*nz4*s1 - nx2*ny3*nz4*s1 - nx4*ny3*nz1*s2 +
                nx3*ny4*nz1*s2 +
                nx4*ny1*nz3*s2 - nx1*ny4*nz3*s2 - nx3*ny1*nz4*s2 +
                nx1*ny3*nz4*s2 +
                nx4*ny2*nz1*s3 - nx2*ny4*nz1*s3 - nx4*ny1*nz2*s3 +
                nx1*ny4*nz2*s3 +
                nx2*ny1*nz4*s3 - nx1*ny2*nz4*s3 - nx3*ny2*nz1*s4 +
                nx2*ny3*nz1*s4 +
                nx3*ny1*nz2*s4 - nx1*ny3*nz2*s4 - nx2*ny1*nz3*s4 +
                nx1*ny2*nz3*s4;

         xins_dx = 
  nx4_dx*ny3*nz2*s1 + nx4*ny3_dx*nz2*s1 + nx4*ny3*nz2_dx*s1 + nx4*ny3*nz2*s1_dx 
- nx3_dx*ny4*nz2*s1 - nx3*ny4_dx*nz2*s1 - nx3*ny4*nz2_dx*s1 - nx3*ny4*nz2*s1_dx 
- nx4_dx*ny2*nz3*s1 - nx4*ny2_dx*nz3*s1 - nx4*ny2*nz3_dx*s1 - nx4*ny2*nz3*s1_dx 
+ nx2_dx*ny4*nz3*s1 + nx2*ny4_dx*nz3*s1 + nx2*ny4*nz3_dx*s1 + nx2*ny4*nz3*s1_dx 
+ nx3_dx*ny2*nz4*s1 + nx3*ny2_dx*nz4*s1 + nx3*ny2*nz4_dx*s1 + nx3*ny2*nz4*s1_dx 
- nx2_dx*ny3*nz4*s1 - nx2*ny3_dx*nz4*s1 - nx2*ny3*nz4_dx*s1 - nx2*ny3*nz4*s1_dx 
- nx4_dx*ny3*nz1*s2 - nx4*ny3_dx*nz1*s2 - nx4*ny3*nz1_dx*s2 - nx4*ny3*nz1*s2_dx 
+ nx3_dx*ny4*nz1*s2 + nx3*ny4_dx*nz1*s2 + nx3*ny4*nz1_dx*s2 + nx3*ny4*nz1*s2_dx 
+ nx4_dx*ny1*nz3*s2 + nx4*ny1_dx*nz3*s2 + nx4*ny1*nz3_dx*s2 + nx4*ny1*nz3*s2_dx 
- nx1_dx*ny4*nz3*s2 - nx1*ny4_dx*nz3*s2 - nx1*ny4*nz3_dx*s2 - nx1*ny4*nz3*s2_dx 
- nx3_dx*ny1*nz4*s2 - nx3*ny1_dx*nz4*s2 - nx3*ny1*nz4_dx*s2 - nx3*ny1*nz4*s2_dx 
+ nx1_dx*ny3*nz4*s2 + nx1*ny3_dx*nz4*s2 + nx1*ny3*nz4_dx*s2 + nx1*ny3*nz4*s2_dx 
+ nx4_dx*ny2*nz1*s3 + nx4*ny2_dx*nz1*s3 + nx4*ny2*nz1_dx*s3 + nx4*ny2*nz1*s3_dx 
- nx2_dx*ny4*nz1*s3 - nx2*ny4_dx*nz1*s3 - nx2*ny4*nz1_dx*s3 - nx2*ny4*nz1*s3_dx 
- nx4_dx*ny1*nz2*s3 - nx4*ny1_dx*nz2*s3 - nx4*ny1*nz2_dx*s3 - nx4*ny1*nz2*s3_dx 
+ nx1_dx*ny4*nz2*s3 + nx1*ny4_dx*nz2*s3 + nx1*ny4*nz2_dx*s3 + nx1*ny4*nz2*s3_dx 
+ nx2_dx*ny1*nz4*s3 + nx2*ny1_dx*nz4*s3 + nx2*ny1*nz4_dx*s3 + nx2*ny1*nz4*s3_dx 
- nx1_dx*ny2*nz4*s3 - nx1*ny2_dx*nz4*s3 - nx1*ny2*nz4_dx*s3 - nx1*ny2*nz4*s3_dx 
- nx3_dx*ny2*nz1*s4 - nx3*ny2_dx*nz1*s4 - nx3*ny2*nz1_dx*s4 - nx3*ny2*nz1*s4_dx 
+ nx2_dx*ny3*nz1*s4 + nx2*ny3_dx*nz1*s4 + nx2*ny3*nz1_dx*s4 + nx2*ny3*nz1*s4_dx 
+ nx3_dx*ny1*nz2*s4 + nx3*ny1_dx*nz2*s4 + nx3*ny1*nz2_dx*s4 + nx3*ny1*nz2*s4_dx 
- nx1_dx*ny3*nz2*s4 - nx1*ny3_dx*nz2*s4 - nx1*ny3*nz2_dx*s4 - nx1*ny3*nz2*s4_dx 
- nx2_dx*ny1*nz3*s4 - nx2*ny1_dx*nz3*s4 - nx2*ny1*nz3_dx*s4 - nx2*ny1*nz3*s4_dx 
+ nx1_dx*ny2*nz3*s4 + nx1*ny2_dx*nz3*s4 + nx1*ny2*nz3_dx*s4 + nx1*ny2*nz3*s4_dx;

         xins_dy = 
  nx4_dy*ny3*nz2*s1 + nx4*ny3_dy*nz2*s1 + nx4*ny3*nz2_dy*s1 + nx4*ny3*nz2*s1_dy 
- nx3_dy*ny4*nz2*s1 - nx3*ny4_dy*nz2*s1 - nx3*ny4*nz2_dy*s1 - nx3*ny4*nz2*s1_dy 
- nx4_dy*ny2*nz3*s1 - nx4*ny2_dy*nz3*s1 - nx4*ny2*nz3_dy*s1 - nx4*ny2*nz3*s1_dy 
+ nx2_dy*ny4*nz3*s1 + nx2*ny4_dy*nz3*s1 + nx2*ny4*nz3_dy*s1 + nx2*ny4*nz3*s1_dy 
+ nx3_dy*ny2*nz4*s1 + nx3*ny2_dy*nz4*s1 + nx3*ny2*nz4_dy*s1 + nx3*ny2*nz4*s1_dy 
- nx2_dy*ny3*nz4*s1 - nx2*ny3_dy*nz4*s1 - nx2*ny3*nz4_dy*s1 - nx2*ny3*nz4*s1_dy 
- nx4_dy*ny3*nz1*s2 - nx4*ny3_dy*nz1*s2 - nx4*ny3*nz1_dy*s2 - nx4*ny3*nz1*s2_dy 
+ nx3_dy*ny4*nz1*s2 + nx3*ny4_dy*nz1*s2 + nx3*ny4*nz1_dy*s2 + nx3*ny4*nz1*s2_dy 
+ nx4_dy*ny1*nz3*s2 + nx4*ny1_dy*nz3*s2 + nx4*ny1*nz3_dy*s2 + nx4*ny1*nz3*s2_dy 
- nx1_dy*ny4*nz3*s2 - nx1*ny4_dy*nz3*s2 - nx1*ny4*nz3_dy*s2 - nx1*ny4*nz3*s2_dy 
- nx3_dy*ny1*nz4*s2 - nx3*ny1_dy*nz4*s2 - nx3*ny1*nz4_dy*s2 - nx3*ny1*nz4*s2_dy 
+ nx1_dy*ny3*nz4*s2 + nx1*ny3_dy*nz4*s2 + nx1*ny3*nz4_dy*s2 + nx1*ny3*nz4*s2_dy 
+ nx4_dy*ny2*nz1*s3 + nx4*ny2_dy*nz1*s3 + nx4*ny2*nz1_dy*s3 + nx4*ny2*nz1*s3_dy 
- nx2_dy*ny4*nz1*s3 - nx2*ny4_dy*nz1*s3 - nx2*ny4*nz1_dy*s3 - nx2*ny4*nz1*s3_dy 
- nx4_dy*ny1*nz2*s3 - nx4*ny1_dy*nz2*s3 - nx4*ny1*nz2_dy*s3 - nx4*ny1*nz2*s3_dy 
+ nx1_dy*ny4*nz2*s3 + nx1*ny4_dy*nz2*s3 + nx1*ny4*nz2_dy*s3 + nx1*ny4*nz2*s3_dy 
+ nx2_dy*ny1*nz4*s3 + nx2*ny1_dy*nz4*s3 + nx2*ny1*nz4_dy*s3 + nx2*ny1*nz4*s3_dy 
- nx1_dy*ny2*nz4*s3 - nx1*ny2_dy*nz4*s3 - nx1*ny2*nz4_dy*s3 - nx1*ny2*nz4*s3_dy 
- nx3_dy*ny2*nz1*s4 - nx3*ny2_dy*nz1*s4 - nx3*ny2*nz1_dy*s4 - nx3*ny2*nz1*s4_dy 
+ nx2_dy*ny3*nz1*s4 + nx2*ny3_dy*nz1*s4 + nx2*ny3*nz1_dy*s4 + nx2*ny3*nz1*s4_dy 
+ nx3_dy*ny1*nz2*s4 + nx3*ny1_dy*nz2*s4 + nx3*ny1*nz2_dy*s4 + nx3*ny1*nz2*s4_dy 
- nx1_dy*ny3*nz2*s4 - nx1*ny3_dy*nz2*s4 - nx1*ny3*nz2_dy*s4 - nx1*ny3*nz2*s4_dy 
- nx2_dy*ny1*nz3*s4 - nx2*ny1_dy*nz3*s4 - nx2*ny1*nz3_dy*s4 - nx2*ny1*nz3*s4_dy 
+ nx1_dy*ny2*nz3*s4 + nx1*ny2_dy*nz3*s4 + nx1*ny2*nz3_dy*s4 + nx1*ny2*nz3*s4_dy;

         xins_dz = 
  nx4_dz*ny3*nz2*s1 + nx4*ny3_dz*nz2*s1 + nx4*ny3*nz2_dz*s1 + nx4*ny3*nz2*s1_dz 
- nx3_dz*ny4*nz2*s1 - nx3*ny4_dz*nz2*s1 - nx3*ny4*nz2_dz*s1 - nx3*ny4*nz2*s1_dz 
- nx4_dz*ny2*nz3*s1 - nx4*ny2_dz*nz3*s1 - nx4*ny2*nz3_dz*s1 - nx4*ny2*nz3*s1_dz 
+ nx2_dz*ny4*nz3*s1 + nx2*ny4_dz*nz3*s1 + nx2*ny4*nz3_dz*s1 + nx2*ny4*nz3*s1_dz 
+ nx3_dz*ny2*nz4*s1 + nx3*ny2_dz*nz4*s1 + nx3*ny2*nz4_dz*s1 + nx3*ny2*nz4*s1_dz 
- nx2_dz*ny3*nz4*s1 - nx2*ny3_dz*nz4*s1 - nx2*ny3*nz4_dz*s1 - nx2*ny3*nz4*s1_dz 
- nx4_dz*ny3*nz1*s2 - nx4*ny3_dz*nz1*s2 - nx4*ny3*nz1_dz*s2 - nx4*ny3*nz1*s2_dz 
+ nx3_dz*ny4*nz1*s2 + nx3*ny4_dz*nz1*s2 + nx3*ny4*nz1_dz*s2 + nx3*ny4*nz1*s2_dz 
+ nx4_dz*ny1*nz3*s2 + nx4*ny1_dz*nz3*s2 + nx4*ny1*nz3_dz*s2 + nx4*ny1*nz3*s2_dz 
- nx1_dz*ny4*nz3*s2 - nx1*ny4_dz*nz3*s2 - nx1*ny4*nz3_dz*s2 - nx1*ny4*nz3*s2_dz 
- nx3_dz*ny1*nz4*s2 - nx3*ny1_dz*nz4*s2 - nx3*ny1*nz4_dz*s2 - nx3*ny1*nz4*s2_dz 
+ nx1_dz*ny3*nz4*s2 + nx1*ny3_dz*nz4*s2 + nx1*ny3*nz4_dz*s2 + nx1*ny3*nz4*s2_dz 
+ nx4_dz*ny2*nz1*s3 + nx4*ny2_dz*nz1*s3 + nx4*ny2*nz1_dz*s3 + nx4*ny2*nz1*s3_dz 
- nx2_dz*ny4*nz1*s3 - nx2*ny4_dz*nz1*s3 - nx2*ny4*nz1_dz*s3 - nx2*ny4*nz1*s3_dz 
- nx4_dz*ny1*nz2*s3 - nx4*ny1_dz*nz2*s3 - nx4*ny1*nz2_dz*s3 - nx4*ny1*nz2*s3_dz 
+ nx1_dz*ny4*nz2*s3 + nx1*ny4_dz*nz2*s3 + nx1*ny4*nz2_dz*s3 + nx1*ny4*nz2*s3_dz 
+ nx2_dz*ny1*nz4*s3 + nx2*ny1_dz*nz4*s3 + nx2*ny1*nz4_dz*s3 + nx2*ny1*nz4*s3_dz 
- nx1_dz*ny2*nz4*s3 - nx1*ny2_dz*nz4*s3 - nx1*ny2*nz4_dz*s3 - nx1*ny2*nz4*s3_dz 
- nx3_dz*ny2*nz1*s4 - nx3*ny2_dz*nz1*s4 - nx3*ny2*nz1_dz*s4 - nx3*ny2*nz1*s4_dz 
+ nx2_dz*ny3*nz1*s4 + nx2*ny3_dz*nz1*s4 + nx2*ny3*nz1_dz*s4 + nx2*ny3*nz1*s4_dz 
+ nx3_dz*ny1*nz2*s4 + nx3*ny1_dz*nz2*s4 + nx3*ny1*nz2_dz*s4 + nx3*ny1*nz2*s4_dz 
- nx1_dz*ny3*nz2*s4 - nx1*ny3_dz*nz2*s4 - nx1*ny3*nz2_dz*s4 - nx1*ny3*nz2*s4_dz 
- nx2_dz*ny1*nz3*s4 - nx2*ny1_dz*nz3*s4 - nx2*ny1*nz3_dz*s4 - nx2*ny1*nz3*s4_dz 
+ nx1_dz*ny2*nz3*s4 + nx1*ny2_dz*nz3*s4 + nx1*ny2*nz3_dz*s4 + nx1*ny2*nz3*s4_dz;

	 xins_dx = ( det * xins_dx - xins * det_dx ) / det / det ;

	 xins_dy = ( det * xins_dy - xins * det_dy ) / det / det ;

	 xins_dz = ( det * xins_dz - xins * det_dz ) / det / det ;

	 xins = xins / det;

	 dARdx[0] = ( circ * xins_dx - xins * circ_dx ) / circ / circ * 3.0;
	 dARdx[1] = ( circ * xins_dy - xins * circ_dy ) / circ / circ * 3.0;
	 dARdx[2] = ( circ * xins_dz - xins * circ_dz ) / circ / circ * 3.0;

	 *ar = xins/circ*3.0;

  return grid;
}

double gridMinVolume( Grid *grid )
{
  int cellId, nodes[4];
  double minVol;
  minVol = 999.0;
  for (cellId=0;cellId<grid->maxcell;cellId++)
    if ( NULL != gridCell( grid, cellId, nodes) )
      minVol = MIN(minVol,gridVolume(grid, nodes) );
  return minVol;
}

bool gridNegCellAroundNode( Grid *grid, int node )
{
  int cellId, nodes[4];
  AdjIterator it;

  for ( it = adjFirst(grid->cellAdj,node); adjValid(it); it = adjNext(it) ) {
    cellId = adjItem(it);
    gridCell( grid, cellId, nodes );
    if (gridVolume(grid, nodes) <= 0.0) return TRUE;
  }

  return FALSE;
}

bool gridNegCellAroundNodeExceptGem( Grid *grid, int node )
{
  int igem, cellId, nodes[4];
  bool inGem;
  AdjIterator it;

  for ( it = adjFirst(grid->cellAdj,node); adjValid(it); it = adjNext(it) ) {
    cellId = adjItem(it);
    inGem = FALSE;
    for ( igem =0; !inGem && igem < grid->ngem ; igem++)
      inGem = inGem || (cellId == grid->gem[igem]);
    if ( !inGem ) {
      gridCell( grid, cellId, nodes );
      if (gridVolume(grid, nodes) <= 0.0) return TRUE;
    }
  }

  return FALSE;
}

double gridMinAR( Grid *grid )
{
  int cellId, nodes[4];
  double minAR;
  minAR = 999.0;
  for (cellId=0;cellId<grid->maxcell;cellId++)
    if ( NULL != gridCell( grid, cellId, nodes) )
      minAR = MIN(minAR, gridAR(grid, nodes) );
  return minAR;
}

bool gridRightHandedFace(Grid *grid, int face ){
  int cell;
  int nodes[4];
  cell = gridFindCellWithFace(grid, face );
  if (cell == EMPTY) {
    printf("gridRightHandedFace: %s: %d: no cell for face %d %d %d %d\n",
	   __FILE__, __LINE__,
	   face,grid->f2n[0+3*face],grid->f2n[1+3*face],grid->f2n[2+3*face]);
    return FALSE;
  }
  nodes[0] = grid->f2n[0+3*face];
  nodes[1] = grid->f2n[1+3*face];
  nodes[2] = grid->f2n[2+3*face];
  nodes[3] 
    = grid->c2n[0+4*cell] 
    + grid->c2n[1+4*cell] 
    + grid->c2n[2+4*cell] 
    + grid->c2n[3+4*cell]
    - nodes[0]
    - nodes[1]
    - nodes[2];

  return (gridVolume(grid, nodes) > 0.0);
}

bool gridRightHandedBoundary( Grid *grid )
{
  int face;
  bool rightHanded;
  rightHanded = TRUE;

  for (face=0;face<grid->maxface;face++)
    if ( grid->f2n[3*face] != EMPTY )
      rightHanded = gridRightHandedFace(grid, face) && rightHanded;

  return rightHanded;
}

double gridFaceArea(Grid *grid, int n0, int n1, int n2 )
{
  int ixyz;
  double edge1[3], edge2[3], norm[3], length; 
  
  for (ixyz = 0 ; ixyz < 3 ; ixyz++ ){
    edge1[ixyz] = grid->xyz[ixyz+3*n1]
                - grid->xyz[ixyz+3*n0];
    edge2[ixyz] = grid->xyz[ixyz+3*n2]
                - grid->xyz[ixyz+3*n0];
  }

  norm[0] = edge1[1]*edge2[2] - edge1[2]*edge2[1]; 
  norm[1] = edge1[2]*edge2[0] - edge1[0]*edge2[2]; 
  norm[2] = edge1[0]*edge2[1] - edge1[1]*edge2[0]; 

  length = sqrt(norm[0]*norm[0] + norm[1]*norm[1] + norm[2]*norm[2]);

  return  0.5*length;
}

double gridFaceAR(Grid *grid, int n0, int n1, int n2 )
{
  int ixyz;
  double e0[3], e1[3], e2[3];
  double l0, l1, l2;
  double n[3], a28, p, ar;

  for (ixyz = 0 ; ixyz < 3 ; ixyz++ ){
    e0[ixyz] = grid->xyz[ixyz+3*n1] - grid->xyz[ixyz+3*n0];
    e1[ixyz] = grid->xyz[ixyz+3*n2] - grid->xyz[ixyz+3*n1];
    e2[ixyz] = grid->xyz[ixyz+3*n0] - grid->xyz[ixyz+3*n2];
  }

  n[0] = e1[1]*e2[2] - e1[2]*e2[1]; 
  n[1] = e1[2]*e2[0] - e1[0]*e2[2]; 
  n[2] = e1[0]*e2[1] - e1[1]*e2[0]; 

  l0 = sqrt(e0[0]*e0[0] + e0[1]*e0[1] + e0[2]*e0[2]);
  l1 = sqrt(e1[0]*e1[0] + e1[1]*e1[1] + e1[2]*e1[2]);
  l2 = sqrt(e2[0]*e2[0] + e2[1]*e2[1] + e2[2]*e2[2]);

  a28 = 4.0*(n[0]*n[0] + n[1]*n[1] + n[2]*n[2]);

  p = l0+l1+l2;

  ar = a28/(p*l0*l1*l2);

  return ar;

}

double gridMinFaceMR( Grid *grid )
{
  int face;
  double minMR;
  minMR = 999.0;
  for (face=0;face<grid->maxface;face++) {
    if ( EMPTY != grid->f2n[3*face]) {
      minMR = MIN(minMR, gridFaceMR(grid, 
				    grid->f2n[0+3*face],
				    grid->f2n[1+3*face],
				    grid->f2n[2+3*face] ) );
    }
  }
  return minMR;
}

#define SQRT3 (1.73205080756888)

double gridFaceMR(Grid *grid, int n0, int n1, int n2 )
{
  double x1, x2, x3; 
  double y1, y2, y3; 
  double z1, z2, z3;
  int i;
  double m[6], m0[6], m1[6], m2[6], j[9];
  
  double ex1, ey1, ez1;
  double ex2, ey2, ez2;
  double ex3, ey3, ez3;
  double nx, ny, nz;
  double l1, l2, l3;
  double a,d;
  double r;

  x1 = grid->xyz[0+3*n0];
  y1 = grid->xyz[1+3*n0];
  z1 = grid->xyz[2+3*n0];

  x2 = grid->xyz[0+3*n1];
  y2 = grid->xyz[1+3*n1];
  z2 = grid->xyz[2+3*n1];

  x3 = grid->xyz[0+3*n2];
  y3 = grid->xyz[1+3*n2];
  z3 = grid->xyz[2+3*n2];
 
  gridMapMatrix(grid,n0,m0);
  gridMapMatrix(grid,n1,m1);
  gridMapMatrix(grid,n2,m2);

  for (i=0;i<6;i++) m[i]=0.333333333333333*(m0[i]+m1[i]+m2[i]);
  if (grid != gridConvertMetricToJacobian(grid, m, j) ) {
    printf("%s: %d: gridFaceMR: gridConvertMetricToJacobian NULL\n",
	   __FILE__,__LINE__);
    printf("m \n %25.15f %25.15f %25.15f \n %25.15f %25.15f %25.15f \n %25.15f %25.15f %25.15f\n",m[0],m[1],m[2],m[1],m[3],m[4],m[2],m[4],m[5]);
  }

  gridMapXYZWithJ(j, &x1, &y1, &z1);
  gridMapXYZWithJ(j, &x2, &y2, &z2);
  gridMapXYZWithJ(j, &x3, &y3, &z3);

  ex1 = x2-x1;
  ey1 = y2-y1;
  ez1 = z2-z1;

  ex2 = x3-x2;
  ey2 = y3-y2;
  ez2 = z3-z2;

  ex3 = x1-x3;
  ey3 = y1-y3;
  ez3 = z1-z3;

  nx = ey1*ez2 - ez1*ey2; 

  ny = ez1*ex2 - ex1*ez2; 

  nz = ex1*ey2 - ey1*ex2; 

  l1 = ex1*ex1 + ey1*ey1 + ez1*ez1;

  l2 = ex2*ex2 + ey2*ey2 + ez2*ez2;

  l3 = ex3*ex3 + ey3*ey3 + ez3*ez3;

  a = sqrt(nx*nx + ny*ny + nz*nz);

  a = 0.5*a;

  d = l1+l2+l3;

  r = a/d;

  return 4*SQRT3*r;

}

Grid *gridFaceMRDerivative(Grid *grid, int* nodes, double *mr, double *dMRdx )
{
  double x1, x2, x3; 
  double y1, y2, y3; 
  double z1, z2, z3;
  int i;
  double m[6], m0[6], m1[6], m2[6], j[9];
  
  x1 = grid->xyz[0+3*nodes[0]];
  y1 = grid->xyz[1+3*nodes[0]];
  z1 = grid->xyz[2+3*nodes[0]];

  x2 = grid->xyz[0+3*nodes[1]];
  y2 = grid->xyz[1+3*nodes[1]];
  z2 = grid->xyz[2+3*nodes[1]];

  x3 = grid->xyz[0+3*nodes[2]];
  y3 = grid->xyz[1+3*nodes[2]];
  z3 = grid->xyz[2+3*nodes[2]];
 
  gridMapMatrix(grid,nodes[0],m0);
  gridMapMatrix(grid,nodes[1],m1);
  gridMapMatrix(grid,nodes[2],m2);

  for (i=0;i<6;i++) m[i]=0.333333333333333*(m0[i]+m1[i]+m2[i]);
  if (grid != gridConvertMetricToJacobian(grid, m, j) ) {
    printf("%s: %d: griFaceMRDerivative: gridConvertMetricToJacobian NULL\n",
	   __FILE__,__LINE__);
  }
  gridConvertMetricToJacobian( grid, m, j );

  gridMapXYZWithJ(j, &x1, &y1, &z1);
  gridMapXYZWithJ(j, &x2, &y2, &z2);
  gridMapXYZWithJ(j, &x3, &y3, &z3);

  FaceMRDerivative(x1,y1,z1,x2,y2,z2,x3,y3,z3,mr,dMRdx );
  return grid;
}

void FaceMRDerivative(double x1, double y1, double z1,
		      double x2, double y2, double z2,
		      double x3, double y3, double z3,
		      double *mr, double *dMRdx  )
{

  double ex1, ey1, ez1;
  double ex2, ey2, ez2;
  double ex3, ey3, ez3;

  double ex1_dx, ey1_dy, ez1_dz;
  double ex3_dx, ey3_dy, ez3_dz;

  double nx, ny, nz;
  double nx_dy,nx_dz;
  double ny_dx,ny_dz;
  double nz_dx,nz_dy;
  double l1, l2, l3;
  double l1_dx, l1_dy, l1_dz;
  double l3_dx, l3_dy, l3_dz;
  double a,d;
  double a_dx, a_dy, a_dz;
  double d_dx, d_dy, d_dz;
  double r;
  double r_dx, r_dy, r_dz;

  ex1 = x2-x1;
  ey1 = y2-y1;
  ez1 = z2-z1;

  ex1_dx = -1.0;
  ey1_dy = -1.0;
  ez1_dz = -1.0;

  ex2 = x3-x2;
  ey2 = y3-y2;
  ez2 = z3-z2;

  ex3 = x1-x3;
  ey3 = y1-y3;
  ez3 = z1-z3;

  ex3_dx = 1.0;
  ey3_dy = 1.0;
  ez3_dz = 1.0;

  nx = ey1*ez2 - ez1*ey2; 
  nx_dy =  ey1_dy * ez2;
  nx_dz = -ez1_dz*ey2; 

  ny = ez1*ex2 - ex1*ez2; 
  ny_dx = -ex1_dx*ez2; 
  ny_dz =  ez1_dz*ex2; 

  nz = ex1*ey2 - ey1*ex2; 
  nz_dx =  ex1_dx*ey2; 
  nz_dy = -ey1_dy*ex2; 

  l1 = ex1*ex1 + ey1*ey1 + ez1*ez1;
  l1_dx = 2*ex1_dx*ex1;
  l1_dy = 2*ey1_dy*ey1;
  l1_dz = 2*ez1_dz*ez1;

  l2 = ex2*ex2 + ey2*ey2 + ez2*ez2;

  l3 = ex3*ex3 + ey3*ey3 + ez3*ez3;
  l3_dx = 2*ex3_dx*ex3;
  l3_dy = 2*ey3_dy*ey3;
  l3_dz = 2*ez3_dz*ez3;

  a = sqrt(nx*nx + ny*ny + nz*nz);
  a_dx = (ny*ny_dx + nz*nz_dx)/a;
  a_dy = (nx*nx_dy + nz*nz_dy)/a;
  a_dz = (nx*nx_dz + ny*ny_dz)/a; 

  a = 0.5*a;
  a_dx = 0.5*a_dx;
  a_dy = 0.5*a_dy;
  a_dz = 0.5*a_dz;

  d = l1+l2+l3;
  d_dx = l1_dx + l3_dx;
  d_dy = l1_dy + l3_dy;
  d_dz = l1_dz + l3_dz;

  r = a/d;
  r_dx = ( d * a_dx - a * d_dx ) / d / d;
  r_dy = ( d * a_dy - a * d_dy ) / d / d;
  r_dz = ( d * a_dz - a * d_dz ) / d / d;

  *mr = 4*SQRT3*r;

  dMRdx[0] = 4*SQRT3*r_dx;
  dMRdx[1] = 4*SQRT3*r_dy;
  dMRdx[2] = 4*SQRT3*r_dz;

}

Grid *gridNodeFaceMR(Grid *grid, int node, double *mr )
{
  AdjIterator it;
  int face;
  double local_mr;

  *mr = 1.0;

  if ( gridNegCellAroundNode( grid, node ) ) {
    *mr = -1.0;
    return grid;
  }

  for ( it = adjFirst(grid->faceAdj,node); adjValid(it); it = adjNext(it) ){
    face = adjItem(it);
    local_mr = gridFaceMR(grid, 
			  grid->f2n[0+3*face],
			  grid->f2n[1+3*face],
			  grid->f2n[2+3*face]);
    if ( local_mr < *mr ) *mr = local_mr;
  }

  return grid;
}

Grid *gridNodeFaceMRDerivative (Grid *grid, int node, double *mr, double *dMRdx )
{
  AdjIterator it;
  int face, nodes[3], index, inode;
  double local_mr, local_dMRdx[3];

  *mr = 1.1;
  dMRdx[0] = DBL_MAX;
  dMRdx[1] = DBL_MAX;
  dMRdx[2] = DBL_MAX;

  for ( it = adjFirst(grid->faceAdj,node); adjValid(it); it = adjNext(it) ){
    face = adjItem(it);
    nodes[0] = node;
    index = 1;
    for (inode=0; inode<3;inode++) {
      if ( node != grid->f2n[inode+3*face]){
	nodes[index] = grid->f2n[inode+3*face];
	index++;
      }
    }

    if ( grid != gridFaceMRDerivative(grid, nodes, &local_mr, local_dMRdx ) ) {
      *mr = 0.0;
      dMRdx[0] = DBL_MAX;
      dMRdx[1] = DBL_MAX;
      dMRdx[2] = DBL_MAX;
      return NULL;
    }
    if ( local_mr < *mr ) {
      *mr = local_mr;
      dMRdx[0] = local_dMRdx[0];
      dMRdx[1] = local_dMRdx[1];
      dMRdx[2] = local_dMRdx[2];
    }
  }

  return grid;
}
