
#include "ruby.h"
#include "ref_cell.h"

#define GET_REF_CELL_FROM_SELF REF_CELL ref_cell; Data_Get_Struct( self, REF_CELL_STRUCT, ref_cell );

static void ref_cell_sweeper( void *ref_cell )
{
  ref_cell_free( (REF_CELL)ref_cell );
}

VALUE ref_cell_new( VALUE class, VALUE node_per )
{
  REF_CELL ref_cell;
  VALUE obj;
  ref_cell_create( NUM2INT(node_per), &ref_cell );
  obj = Data_Wrap_Struct( class, NULL, ref_cell_sweeper, ref_cell );
  return obj;
}

VALUE rb_ref_cell_n( VALUE self )
{
  GET_REF_CELL_FROM_SELF;
  return INT2NUM( ref_cell_n(ref_cell) );
}

VALUE rb_ref_cell_add( VALUE self, VALUE rb_nodes )
{
  REF_INT len, i;
  REF_INT *nodes;
  REF_STATUS code;
  GET_REF_CELL_FROM_SELF;
  len = RARRAY_LEN(rb_nodes);
  nodes = (REF_INT *)malloc(len*sizeof(REF_INT));
  for(i=0;i<len;i++) nodes[i] = NUM2INT( rb_ary_entry( rb_nodes, i) );
  code = ref_cell_add(ref_cell,nodes);
  free(nodes);
  return INT2NUM(code);
}

VALUE rb_ref_cell_nodes( VALUE self, VALUE rb_cell )
{
  REF_INT len, i;
  REF_INT *nodes;
  REF_STATUS code;
  VALUE rb_nodes;
  GET_REF_CELL_FROM_SELF;
  nodes = (REF_INT *)malloc(ref_cell_node_per(ref_cell)*sizeof(REF_INT));
  code = ref_cell_nodes(ref_cell,NUM2INT(rb_cell),nodes);
  if ( REF_SUCCESS != code ) 
    {
      free(nodes);
      return Qnil;
    }
  rb_nodes= rb_ary_new2(ref_cell_node_per(ref_cell));
  for (i=0;i<ref_cell_node_per(ref_cell);i++) 
    rb_ary_store( rb_nodes, i, INT2NUM(nodes[i]) );
  free(nodes);
  return rb_nodes;
}

VALUE ref_cell_class;

void Init_ref_cell()
{
  ref_cell_class = rb_define_class( "Ref_Cell", rb_cObject );
  rb_define_singleton_method( ref_cell_class, "new", ref_cell_new, 1 );
  rb_define_method( ref_cell_class, "n", rb_ref_cell_n, 0 );
  rb_define_method( ref_cell_class, "add", rb_ref_cell_add, 1 );
  rb_define_method( ref_cell_class, "nodes", rb_ref_cell_nodes, 1 );
}
