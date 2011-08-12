/*******************************************************************\

Module: Value Set (Flow Insensitive, Sharing)

Author: Daniel Kroening, kroening@kroening.com

\*******************************************************************/

#include <assert.h>

#include <context.h>
#include <simplify_expr.h>
#include <expr_util.h>
#include <base_type.h>
#include <std_expr.h>
#include <i2string.h>
#include <prefix.h>
#include <std_code.h>
#include <arith_tools.h>

#include <langapi/language_util.h>
#include <ansi-c/c_types.h>

#include "value_set_fi.h"

const value_set_fit::object_map_dt value_set_fit::object_map_dt::empty;
object_numberingt value_set_fit::object_numbering;
hash_numbering<irep_idt, irep_id_hash> value_set_fit::function_numbering;

static std::string alloc_adapter_prefix = "alloc_adaptor::";

#define forall_objects(it, map) \
  for(object_map_dt::const_iterator (it) = (map).begin(); \
  (it)!=(map).end(); \
  (it)++)

#define Forall_objects(it, map) \
  for(object_map_dt::iterator (it) = (map).begin(); \
  (it)!=(map).end(); \
  (it)++)
   
/*******************************************************************\

Function: value_set_fit::output

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void value_set_fit::output(
  const namespacet &ns,
  std::ostream &out) const
{
  for(valuest::const_iterator
      v_it=values.begin();
      v_it!=values.end();
      v_it++)
  {
    irep_idt identifier, display_name;
    
    const entryt &e=v_it->second;
  
    if(has_prefix(id2string(e.identifier), "value_set::dynamic_object"))
    {
      display_name=id2string(e.identifier)+e.suffix;
      identifier="";
    }
    else
    {
      #if 0
      const symbolt &symbol=ns.lookup(id2string(e.identifier));
      display_name=symbol.display_name()+e.suffix;
      identifier=symbol.name;
      #else
      identifier=id2string(e.identifier);
      display_name=id2string(identifier)+e.suffix;
      #endif
    }
    
    out << display_name;

    out << " = { ";
    
    object_mapt object_map;
    flatten(e, object_map);
    
    unsigned width=0;    
    
    forall_objects(o_it, object_map.read())
    {
      const exprt &o=object_numbering[o_it->first];
    
      std::string result;

      if(o.id()=="invalid" || o.id()=="unknown")
      {
        result="<";
        result+=from_expr(ns, identifier, o);
        result+=", *, "; // offset unknown
        if (o.type().id()=="unknown")
          result+="*";
        else
          result+=from_type(ns, identifier, o.type());        
        result+=">";
      }
      else
      {
        result="<"+from_expr(ns, identifier, o)+", ";
      
        if(o_it->second.offset_is_set)
          result+=integer2string(o_it->second.offset)+"";
        else
          result+="*";
        
        result+=", ";
        
        if (o.type().id()=="unknown")
          result+="*";
        else
          result+=from_type(ns, identifier, o.type());
      
        result+=">";
      }

      out << result;

      width+=result.size();
    
      object_map_dt::const_iterator next(o_it);
      next++;

      if(next!=object_map.read().end())
      {
        out << ", ";
        if(width>=40) out << "\n      ";
      }
    }

    out << " } " << std::endl;
  }
}

/*******************************************************************\

Function: value_set_fit::flatten

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void value_set_fit::flatten(
  const entryt &e, 
  object_mapt &dest) const 
{  
  #if 0
  std::cout << "FLATTEN: " << e.identifier << e.suffix << std::endl;
  #endif
  
  flatten_seent seen;
  flatten_rec(e, dest, seen);
  
  #if 0
  std::cout << "FLATTEN: Done." << std::endl;
  #endif
}

/*******************************************************************\

Function: value_set_fit::flatten_rec

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void value_set_fit::flatten_rec(
  const entryt &e, 
  object_mapt &dest,
  flatten_seent &seen) const
{    
  #if 0
  std::cout << "FLATTEN_REC: " << e.identifier << e.suffix << std::endl;
  #endif

  std::string identifier = e.identifier.as_string();
  assert(seen.find(identifier + e.suffix)==seen.end());
  
  bool generalize_index = false; 

  seen.insert(identifier + e.suffix);

  forall_objects(it, e.object_map.read())
  {
    const exprt& o=object_numbering[it->first];
        
    if (o.type().id()=="#REF#")
    {
      if (seen.find(o.identifier())!=seen.end())
      {
        generalize_index = true;
        continue;
      }
      
      valuest::const_iterator fi = values.find(o.identifier());
      if (fi==values.end())
      {
        // this is some static object, keep it in.        
        exprt se("symbol", o.type().subtype());
        se.identifier(o.identifier());
        insert(dest, se, 0);
      }
      else
      {
        object_mapt temp;
        flatten_rec(fi->second, temp, seen);
        
        for(object_map_dt::iterator t_it=temp.write().begin();
            t_it!=temp.write().end();
            t_it++)
        {
          if(t_it->second.offset_is_set && 
             it->second.offset_is_set)
          {
            t_it->second.offset += it->second.offset; 
          }
          else
            t_it->second.offset_is_set=false;
        }
        
        forall_objects(oit, temp.read())
          insert(dest, oit);
      }
      
    }
    else 
      insert(dest, it);
  }  
  
  if (generalize_index) // this means we had recursive symbols in there
  {    
    Forall_objects(it, dest.write())
      it->second.offset_is_set = false;
  }
  
  seen.erase(identifier + e.suffix);
} 

/*******************************************************************\

Function: value_set_fit::to_expr

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

exprt value_set_fit::to_expr(object_map_dt::const_iterator it) const
{
  const exprt &object=object_numbering[it->first];
  
  if(object.id()=="invalid" ||
     object.id()=="unknown")
    return object;

  object_descriptor_exprt od;

  od.object()=object;
  
  if(it->second.offset_is_set)
    od.offset()=from_integer(it->second.offset, index_type());

  od.type()=od.object().type();

  return od;
}

/*******************************************************************\

Function: value_set_fit::make_union

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

bool value_set_fit::make_union(const value_set_fit::valuest &new_values)
{
  assert(0);
  bool result=false;
  
  for(valuest::const_iterator
      it=new_values.begin();
      it!=new_values.end();
      it++)
  {
    valuest::iterator it2=values.find(it->first);

    if(it2==values.end())
    {
      // we always track these
      if(has_prefix(id2string(it->second.identifier), 
                    "value_set::dynamic_object") ||
         has_prefix(id2string(it->second.identifier),
                    "value_set::return_value"))
      {
        values.insert(*it);
        result=true;
      }

      continue;
    }
      
    entryt &e=it2->second;
    const entryt &new_e=it->second;
    
    if(make_union(e.object_map, new_e.object_map))
      result=true;
  }
  
  changed = result;
  
  return result;
}

/*******************************************************************\

Function: value_set_fit::make_union

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

bool value_set_fit::make_union(object_mapt &dest, const object_mapt &src) const
{
  bool result=false;
  
  forall_objects(it, src.read())
  {
    if(insert(dest, it))
      result=true;
  }
  
  return result;
}

/*******************************************************************\

Function: value_set_fit::get_value_set

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void value_set_fit::get_value_set(
  const exprt &expr,
  std::list<exprt> &value_set,
  const namespacet &ns) const
{
  object_mapt object_map;
  get_value_set(expr, object_map, ns);
  
  object_mapt flat_map;
  
  forall_objects(it, object_map.read())
  {
    const exprt &object=object_numbering[it->first];
    if (object.type().id()=="#REF#")
    {     
      assert(object.is_symbol());
      
      const irep_idt &ident = object.identifier();
      valuest::const_iterator v_it = values.find(ident);

      if (v_it!=values.end())
      {
        object_mapt temp;
        flatten(v_it->second, temp);
        
        for(object_map_dt::iterator t_it=temp.write().begin();
            t_it!=temp.write().end();
            t_it++)
        {
          if(t_it->second.offset_is_set && 
             it->second.offset_is_set)
          {
            t_it->second.offset += it->second.offset; 
          }
          else
            t_it->second.offset_is_set=false;
          
          flat_map.write()[t_it->first]=t_it->second;
        }        
      }
    }
    else
      flat_map.write()[it->first]=it->second;
  }
  
  forall_objects(fit, flat_map.read())
    value_set.push_back(to_expr(fit));
  
  #if 0
  // Sanity check!
  for(std::list<exprt>::const_iterator it=value_set.begin(); 
      it!=value_set.end(); 
      it++)
    assert(it->type().id()!="#REF");
  #endif

  #if 0
  for(expr_sett::const_iterator it=value_set.begin(); it!=value_set.end(); it++)
    std::cout << "GET_VALUE_SET: " << from_expr(ns, "", *it) << std::endl;
  #endif
}

/*******************************************************************\

Function: value_set_fit::get_value_set

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void value_set_fit::get_value_set(
  const exprt &expr,
  object_mapt &dest,
  const namespacet &ns) const
{
  exprt tmp(expr);
  simplify(tmp);
  
  gvs_recursion_sett recset;
  get_value_set_rec(tmp, dest, "", tmp.type(), ns, recset);
}

/*******************************************************************\

Function: value_set_fit::get_value_set_rec

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void value_set_fit::get_value_set_rec(
  const exprt &expr,
  object_mapt &dest,
  const std::string &suffix,
  const typet &original_type,
  const namespacet &ns,
  gvs_recursion_sett &recursion_set) const
{
  #if 0
  std::cout << "GET_VALUE_SET_REC EXPR: " << from_expr(ns, "", expr) << std::endl;
  std::cout << "GET_VALUE_SET_REC SUFFIX: " << suffix << std::endl;
  std::cout << std::endl;
  #endif

  if(expr.type().id()=="#REF#")
  {
    valuest::const_iterator fi = values.find(expr.identifier());
    
    if(fi!=values.end())
    {
      forall_objects(it, fi->second.object_map.read())   
        get_value_set_rec(object_numbering[it->first], dest, suffix, 
                          original_type, ns, recursion_set);
      return;
    }
    else
    {
      insert(dest, exprt("unknown", original_type));
      return;
    }
  }
  else if(expr.id()=="unknown" || expr.id()=="invalid")
  {
    insert(dest, exprt("unknown", original_type));
    return;
  }  
  else if(expr.is_index())
  {
    assert(expr.operands().size()==2);

    const typet &type=ns.follow(expr.op0().type());

    assert(type.is_array() ||
           type.is_incomplete_array() || 
           type.id()=="#REF#");
           
    get_value_set_rec(expr.op0(), dest, "[]"+suffix, 
                      original_type, ns, recursion_set);
    
    return;
  }
  else if(expr.is_member())
  {
    assert(expr.operands().size()==1);

    if(expr.op0().is_not_nil())   
    {
      const typet &type=ns.follow(expr.op0().type());
  
      assert(type.is_struct() ||
             type.id()=="union" ||
             type.id()=="incomplete_struct" ||
             type.id()=="incomplete_union");
             
      const std::string &component_name=
        expr.component_name().as_string();
      
      get_value_set_rec(expr.op0(), dest, "."+component_name+suffix, 
                        original_type, ns, recursion_set);
        
      return;
    }
  }
  else if(expr.is_symbol())
  {
    // just keep a reference to the ident in the set
    // (if it exists)
    irep_idt ident = expr.identifier().as_string()+suffix;
    valuest::const_iterator v_it=values.find(ident);
    
    if(has_prefix(id2string(ident), alloc_adapter_prefix))
    {
      insert(dest, expr, 0);
      return;
    }
    else if(v_it!=values.end())
    {
      typet t("#REF#");
      t.subtype() = expr.type();
      symbol_exprt sym(ident, t);
      insert(dest, sym, 0);
      return;
    }        
  }
  else if(expr.id()=="if")
  {
    if(expr.operands().size()!=3)
      throw "if takes three operands";

    get_value_set_rec(expr.op1(), dest, suffix, 
                      original_type, ns, recursion_set);
    get_value_set_rec(expr.op2(), dest, suffix, 
                      original_type, ns, recursion_set);

    return;
  }
  else if(expr.is_address_of())
  {
    if(expr.operands().size()!=1)
      throw expr.id_string()+" expected to have one operand";
      
    get_reference_set_sharing(expr.op0(), dest, ns);
    
    return;
  }
  else if(expr.is_dereference() ||
          expr.id()=="implicit_dereference")
  {
    object_mapt reference_set;
    get_reference_set_sharing(expr, reference_set, ns);
    const object_map_dt &object_map=reference_set.read();
    
    if(object_map.begin()!=object_map.end())
    {      
      forall_objects(it1, object_map)
      {
        const exprt &object=object_numbering[it1->first];
        get_value_set_rec(object, dest, suffix, 
                          original_type, ns, recursion_set);
      }

      return;
    }
  }
  else if(expr.id()=="reference_to")
  {
    object_mapt reference_set;
    
    get_reference_set_sharing(expr, reference_set, ns);
    
    const object_map_dt &object_map=reference_set.read();
 
    if(object_map.begin()!=object_map.end())
    {      
      forall_objects(it, object_map)
      {
        const exprt &object=object_numbering[it->first];
        get_value_set_rec(object, dest, suffix, 
                          original_type, ns, recursion_set);
      }

      return;
    }
  }
  else if(expr.is_constant())
  {
    // check if NULL
    if(expr.value()=="NULL" && expr.type().is_pointer())
    {
      insert(dest, exprt("NULL-object", expr.type().subtype()), 0);
      return;
    }
  }
  else if(expr.id()=="typecast")
  {
    if(expr.operands().size()!=1)
      throw "typecast takes one operand";

    get_value_set_rec(expr.op0(), dest, suffix, 
                      original_type, ns, recursion_set);
    
    return;
  }
  else if(expr.id()=="+" || expr.id()=="-")
  {
    if(expr.operands().size()<2)
      throw expr.id_string()+" expected to have at least two operands";

    if(expr.type().is_pointer())
    {
      // find the pointer operand
      const exprt *ptr_operand=NULL;

      forall_operands(it, expr)
        if(it->type().is_pointer())
        {
          if(ptr_operand==NULL)
            ptr_operand=&(*it);
          else
            throw "more than one pointer operand in pointer arithmetic";
        }

      if(ptr_operand==NULL)
        throw "pointer type sum expected to have pointer operand";

      object_mapt pointer_expr_set;
      get_value_set_rec(*ptr_operand, pointer_expr_set, "", 
                        ptr_operand->type(), ns, recursion_set);

      forall_objects(it, pointer_expr_set.read())
      {
        objectt object=it->second;
      
        if(object.offset_is_zero() &&
           expr.operands().size()==2)
        {
          if(!expr.op0().type().is_pointer())
          {
            mp_integer i;
            if(to_integer(expr.op0(), i))
              object.offset_is_set=false;
            else              
              object.offset=(expr.id()=="+")? i : -i;
          }
          else
          {
            mp_integer i;
            if(to_integer(expr.op1(), i))
              object.offset_is_set=false;
            else
              object.offset=(expr.id()=="+")? i : -i;
          }
        }
        else
          object.offset_is_set=false;
          
        insert(dest, it->first, object);
      }

      return;
    }
  }
  else if(expr.id()=="sideeffect")
  {
    const irep_idt &statement=expr.statement();
    
    if(statement=="function_call")
    {
      // these should be gone
      throw "unexpected function_call sideeffect";
    }
    else if(statement=="malloc")
    {
      if(!expr.type().is_pointer())
        throw "malloc expected to return pointer type";
      
      assert(suffix=="");
      
      const typet &dynamic_type=
        static_cast<const typet &>(expr.cmt_type());

      dynamic_object_exprt dynamic_object(dynamic_type);
      // let's make up a `unique' number for this object...
      dynamic_object.instance()=from_integer( 
                   (from_function << 16) | from_target_index, typet("natural"));
      dynamic_object.valid()=true_exprt();

      insert(dest, dynamic_object, 0);
      return;          
    }
    else if(statement=="cpp_new" ||
            statement=="cpp_new[]")
    {
      assert(suffix=="");
      assert(expr.type().is_pointer());

      dynamic_object_exprt dynamic_object(expr.type().subtype());
      dynamic_object.instance()=from_integer(
                   (from_function << 16) | from_target_index, typet("natural"));
      dynamic_object.valid()=true_exprt();

      insert(dest, dynamic_object, 0);
      return;
    }
  }
  else if(expr.is_struct())
  {
    // this is like a static struct object
    insert(dest, address_of_exprt(expr), 0);
    return;
  }
  else if(expr.id()=="with" ||          
          expr.id()=="array_of" ||
          expr.is_array())
  {
    // these are supposed to be done by assign()
    throw "unexpected value in get_value_set: "+expr.id_string();
  }
  else if(expr.id()=="dynamic_object")
  {
    const dynamic_object_exprt &dynamic_object=
      to_dynamic_object_expr(expr);
  
    const std::string name=
      "value_set::dynamic_object"+
      dynamic_object.instance().value().as_string()+suffix;
  
    // look it up
    valuest::const_iterator v_it=values.find(name);

    if(v_it!=values.end())
    {
      make_union(dest, v_it->second.object_map);
      return;
    }
  }

  insert(dest, exprt("unknown", original_type));
}

/*******************************************************************\

Function: value_set_fit::dereference_rec

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void value_set_fit::dereference_rec(
  const exprt &src,
  exprt &dest) const
{
  // remove pointer typecasts
  if(src.id()=="typecast")
  {
    assert(src.type().is_pointer());

    if(src.operands().size()!=1)
      throw "typecast expects one operand";
    
    dereference_rec(src.op0(), dest);
  }
  else
    dest=src;
}

/*******************************************************************\

Function: value_set_fit::get_reference_set

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void value_set_fit::get_reference_set(
  const exprt &expr,
  expr_sett &dest,
  const namespacet &ns) const
{
  object_mapt object_map;
  get_reference_set_sharing(expr, object_map, ns);    
  
  forall_objects(it, object_map.read())
  {    
    const exprt& expr = object_numbering[it->first];
    
    if (expr.type().id()=="#REF#")
    {
      const irep_idt& ident = expr.identifier();
      valuest::const_iterator vit = values.find(ident);
      if (vit==values.end())
      {
        // Assume the variable never was assigned, 
        // so assume it's reference set is unknown.
        dest.insert(exprt("unknown", expr.type()));
      }
      else
      {        
        object_mapt omt;
        flatten(vit->second, omt);
        
        for(object_map_dt::iterator t_it=omt.write().begin();
            t_it!=omt.write().end();
            t_it++)
        {
          if(t_it->second.offset_is_set && 
             it->second.offset_is_set)
          {
            t_it->second.offset += it->second.offset; 
          }
          else
            t_it->second.offset_is_set=false;
        }
        
        forall_objects(it, omt.read())
          dest.insert(to_expr(it));  
      }
    }
    else
      dest.insert(to_expr(it));
  }
}

/*******************************************************************\

Function: value_set_fit::get_reference_set_sharing

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void value_set_fit::get_reference_set_sharing(
  const exprt &expr,
  expr_sett &dest,
  const namespacet &ns) const
{
  object_mapt object_map;
  get_reference_set_sharing(expr, object_map, ns);

  forall_objects(it, object_map.read())
    dest.insert(to_expr(it));
}

/*******************************************************************\

Function: value_set_fit::get_reference_set_sharing_rec

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void value_set_fit::get_reference_set_sharing_rec(
  const exprt &expr,
  object_mapt &dest,
  const namespacet &ns) const
{
  #if 0
  std::cout << "GET_REFERENCE_SET_REC EXPR: " << from_expr(ns, "", expr) << std::endl;
  #endif

  if(expr.type().id()=="#REF#")
  {
    valuest::const_iterator fi = values.find(expr.identifier());
    if(fi!=values.end())
    {      
      forall_objects(it, fi->second.object_map.read())      
        get_reference_set_sharing_rec(object_numbering[it->first], dest, ns);
      return;
    }
  }  
  else if(expr.is_symbol() ||
          expr.id()=="dynamic_object" ||
          expr.id()=="string-constant")
  {    
    if(expr.type().is_array() &&
       expr.type().subtype().is_array())
      insert(dest, expr);
    else    
      insert(dest, expr, 0);

    return;
  }
  else if(expr.is_dereference() ||
          expr.id()=="implicit_dereference")
  {
    if(expr.operands().size()!=1)
      throw expr.id_string()+" expected to have one operand";
    
    gvs_recursion_sett recset;
    object_mapt temp;
    get_value_set_rec(expr.op0(), temp, "", expr.op0().type(), ns, recset);
    
    // REF's need to be dereferenced manually!
    forall_objects(it, temp.read())
    {
      const exprt &obj = object_numbering[it->first];
      if (obj.type().id()=="#REF#")
      {
        const irep_idt &ident = obj.identifier();
        valuest::const_iterator v_it = values.find(ident);
          
        if (v_it!=values.end())
        {
          object_mapt t2;
          flatten(v_it->second, t2);
          
          for(object_map_dt::iterator t_it=t2.write().begin();
              t_it!=t2.write().end();
              t_it++)
          {
            if(t_it->second.offset_is_set && 
               it->second.offset_is_set)
            {
              t_it->second.offset += it->second.offset; 
            }
            else
              t_it->second.offset_is_set=false;
          }
          
          forall_objects(it2, t2.read())
            insert(dest, it2);
        }
        else
          insert(dest, exprt("unknown", obj.type().subtype()));
      }
      else
        insert(dest, it);
    }

    #if 0
    for(expr_sett::const_iterator it=value_set.begin(); it!=value_set.end(); it++)
      std::cout << "VALUE_SET: " << from_expr(ns, "", *it) << std::endl;
    #endif

    return;
  }
  else if(expr.is_index())
  {
    if(expr.operands().size()!=2)
      throw "index expected to have two operands";
  
    const exprt &array=expr.op0();
    const exprt &offset=expr.op1();
    const typet &array_type=ns.follow(array.type());
    
    assert(array_type.is_array() ||
           array_type.is_incomplete_array());

    object_mapt array_references;
    get_reference_set_sharing(array, array_references, ns);
    
    const object_map_dt &object_map=array_references.read();

    forall_objects(a_it, object_map)
    {
      const exprt &object=object_numbering[a_it->first];

      if(object.id()=="unknown")
        insert(dest, exprt("unknown", expr.type()));
      else
      {
        exprt index_expr("index", expr.type());
        index_expr.operands().resize(2);
        index_expr.op0()=object;
        index_expr.op1()=gen_zero(index_type());
        
        // adjust type?
        if(object.type().id()!="#REF#" && 
           ns.follow(object.type())!=array_type)
          index_expr.make_typecast(array.type());
        
        objectt o=a_it->second;
        mp_integer i;

        if(offset.is_zero())
        {
        }
        else if(!to_integer(offset, i) &&
                o.offset_is_zero())
          o.offset=i;
        else
          o.offset_is_set=false;
          
        insert(dest, index_expr, o);
      }
    }
    
    return;
  }
  else if(expr.is_member())
  {
    const irep_idt &component_name=expr.component_name();

    if(expr.operands().size()!=1)
      throw "member expected to have one operand";
  
    const exprt &struct_op=expr.op0();
    
    object_mapt struct_references;
    get_reference_set_sharing(struct_op, struct_references, ns);
    
    forall_objects(it, struct_references.read())
    {
      const exprt &object=object_numbering[it->first];    
      const typet &obj_type=ns.follow(object.type());
      
      if(object.id()=="unknown")
        insert(dest, exprt("unknown", expr.type()));
      else if(object.id()=="dynamic_object" &&
              !obj_type.is_struct() && 
              obj_type.id()!="union")
      {
        // we catch dynamic objects of the wrong type,
        // to avoid non-integral typecasts.        
        insert(dest, exprt("unknown", expr.type()));
      }
      else
      {
        objectt o=it->second;

        exprt member_expr("member", expr.type());
        member_expr.copy_to_operands(object);
        member_expr.component_name(component_name);
        
        // adjust type?
        if(ns.follow(struct_op.type())!=ns.follow(object.type()))
          member_expr.op0().make_typecast(struct_op.type());
        
        insert(dest, member_expr, o);
      }
    }

    return;
  }
  else if(expr.id()=="if")
  {
    if(expr.operands().size()!=3)
      throw "if takes three operands";

    get_reference_set_sharing_rec(expr.op1(), dest, ns);
    get_reference_set_sharing_rec(expr.op2(), dest, ns);
    return;
  }

  insert(dest, exprt("unknown", expr.type()));
}

/*******************************************************************\

Function: value_set_fit::assign

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void value_set_fit::assign(
  const exprt &lhs,
  const exprt &rhs,
  const namespacet &ns)
{
  #if 0
  std::cout << "ASSIGN LHS: " << from_expr(ns, "", lhs) << std::endl;
  std::cout << "ASSIGN RHS: " << from_expr(ns, "", rhs) << std::endl;  
  #endif
  
  if(rhs.id()=="if")
  {
    if(rhs.operands().size()!=3)
      throw "if takes three operands";

    assign(lhs, rhs.op1(), ns);
    assign(lhs, rhs.op2(), ns);
    return;
  }

  const typet &type=ns.follow(lhs.type());
  
  if(type.is_struct() ||
     type.id()=="union")
  {
    const struct_typet &struct_type=to_struct_type(type);
    
    unsigned no=0;
    
    for(struct_typet::componentst::const_iterator
        c_it=struct_type.components().begin();
        c_it!=struct_type.components().end();
        c_it++, no++)
    {
      const typet &subtype=c_it->type();
      const irep_idt &name=c_it->name();

      // ignore methods
      if(subtype.is_code()) continue;
    
      exprt lhs_member("member", subtype);
      lhs_member.component_name(name);
      lhs_member.copy_to_operands(lhs);

      exprt rhs_member;
    
      if(rhs.id()=="unknown" ||
         rhs.id()=="invalid")
      {
        rhs_member=exprt(rhs.id(), subtype);
      }
      else
      {
        assert(base_type_eq(rhs.type(), type, ns));
      
        if(rhs.is_struct() ||
           rhs.is_constant())
        {
          assert(no<rhs.operands().size());
          rhs_member=rhs.operands()[no];
        }
        else if(rhs.id()=="with")
        {
          assert(rhs.operands().size()==3);

          // see if op1 is the member we want
          const exprt &member_operand=rhs.op1();

          const irep_idt &component_name=
            member_operand.component_name();

          if(component_name==name)
          {
            // yes! just take op2
            rhs_member=rhs.op2();
          }
          else
          {
            // no! do op0
            rhs_member=exprt("member", subtype);
            rhs_member.copy_to_operands(rhs.op0());
            rhs_member.component_name(name);
          }
        }
        else
        {
          rhs_member=exprt("member", subtype);
          rhs_member.copy_to_operands(rhs);
          rhs_member.component_name(name);
        }

        assign(lhs_member, rhs_member, ns);
      }
    }
  }
  else if(type.is_array())
  {
    exprt lhs_index("index", type.subtype());
    lhs_index.copy_to_operands(lhs, exprt("unknown", index_type()));

    if(rhs.id()=="unknown" ||
       rhs.id()=="invalid")
    {
      assign(lhs_index, exprt(rhs.id(), type.subtype()), ns);
    }
    else
    {
      assert(base_type_eq(rhs.type(), type, ns));
        
      if(rhs.id()=="array_of")
      {
        assert(rhs.operands().size()==1);
        assign(lhs_index, rhs.op0(), ns);
      }
      else if(rhs.is_array() ||
              rhs.is_constant())
      {
        forall_operands(o_it, rhs)
        {
          assign(lhs_index, *o_it, ns);          
        }
      }
      else if(rhs.id()=="with")
      {
        assert(rhs.operands().size()==3);

        exprt op0_index("index", type.subtype());
        op0_index.copy_to_operands(rhs.op0(), exprt("unknown", index_type()));

        assign(lhs_index, op0_index, ns);
        assign(lhs_index, rhs.op2(), ns);
      }
      else
      {
        exprt rhs_index("index", type.subtype());
        rhs_index.copy_to_operands(rhs, exprt("unknown", index_type()));
        assign(lhs_index, rhs_index, ns);
      }
    }
  }
  else
  {
    // basic type
    object_mapt values_rhs;
    
    get_value_set(rhs, values_rhs, ns);
    
    assign_recursion_sett recset;
    assign_rec(lhs, values_rhs, "", ns, recset);
  }
}

/*******************************************************************\

Function: value_set_fit::do_free

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void value_set_fit::do_free(
  const exprt &op,
  const namespacet &ns)
{
  // op must be a pointer
  if(!op.type().is_pointer())
    throw "free expected to have pointer-type operand";

  // find out what it points to    
  object_mapt value_set;
  get_value_set(op, value_set, ns);
  entryt e; e.identifier="VP:TEMP";
  e.object_map = value_set;
  flatten(e, value_set);
  
  const object_map_dt &object_map=value_set.read();
  
  // find out which *instances* interest us
  expr_sett to_mark;

  forall_objects(it, object_map)
  {
    const exprt &object=object_numbering[it->first];

    if(object.id()=="dynamic_object")
    {
      const dynamic_object_exprt &dynamic_object=
        to_dynamic_object_expr(object);
      
      if(dynamic_object.valid().is_true())
        to_mark.insert(dynamic_object.instance());
    }
  }
  
  // mark these as 'may be invalid'
  // this, unfortunately, destroys the sharing
  for(valuest::iterator v_it=values.begin();
      v_it!=values.end();
      v_it++)
  {
    object_mapt new_object_map;

    const object_map_dt &old_object_map=
      v_it->second.object_map.read();
      
    bool changed=false;

    forall_objects(o_it, old_object_map)
    {
      const exprt &object=object_numbering[o_it->first];

      if(object.id()=="dynamic_object")
      {
        const exprt &instance=
          to_dynamic_object_expr(object).instance();

        if(to_mark.count(instance)==0)
          set(new_object_map, o_it);
        else
        {
          // adjust
          objectt o=o_it->second;
          exprt tmp(object);
          to_dynamic_object_expr(tmp).valid()=exprt("unknown");
          insert(new_object_map, tmp, o);
          changed=true;
        }
      }
      else
        set(new_object_map, o_it);
    }
    
    if(changed)
      v_it->second.object_map=new_object_map;
  }
}

/*******************************************************************\

Function: value_set_fit::assign_rec

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void value_set_fit::assign_rec(
  const exprt &lhs,
  const object_mapt &values_rhs,
  const std::string &suffix,
  const namespacet &ns,
  assign_recursion_sett &recursion_set)
{
  #if 0
  std::cout << "ASSIGN_REC LHS: " << from_expr(ns, "", lhs) << std::endl;
  std::cout << "ASSIGN_REC SUFFIX: " << suffix << std::endl;

  for(object_map_dt::const_iterator it=values_rhs.read().begin(); 
      it!=values_rhs.read().end(); it++)
    std::cout << "ASSIGN_REC RHS: " << to_expr(it) << std::endl;
  #endif

  if(lhs.type().id()=="#REF#")
  {
    const irep_idt &ident = lhs.identifier();
    object_mapt temp;
    gvs_recursion_sett recset;
    get_value_set_rec(lhs, temp, "", lhs.type().subtype(), ns, recset);
    
    if(recursion_set.find(ident)!=recursion_set.end())
    {
      recursion_set.insert(ident);
      
      forall_objects(it, temp.read())
        if(object_numbering[it->first].id()!="unknown")
          assign_rec(object_numbering[it->first], values_rhs, 
                     suffix, ns, recursion_set);
      
      recursion_set.erase(ident);
    }
  }
  else if(lhs.is_symbol())
  {
    const irep_idt &identifier=lhs.identifier();
    
    if(has_prefix(id2string(identifier), 
                  "value_set::dynamic_object") ||
       has_prefix(id2string(identifier),
                  "value_set::return_value") ||
       values.find(id2string(identifier)+suffix)!=values.end())
       // otherwise we don't track this value 
    {    
      entryt &entry = get_entry(identifier, suffix);
          
      if (make_union(entry.object_map, values_rhs))
        changed = true;
    }
  }
  else if(lhs.id()=="dynamic_object")
  {
    const dynamic_object_exprt &dynamic_object=
      to_dynamic_object_expr(lhs);
  
    const std::string name=
      "value_set::dynamic_object"+
      dynamic_object.instance().value().as_string();

    if (make_union(get_entry(name, suffix).object_map, values_rhs))
      changed = true;
  }
  else if(lhs.is_dereference() ||
          lhs.id()=="implicit_dereference")
  {
    if(lhs.operands().size()!=1)
      throw lhs.id_string()+" expected to have one operand";
      
    object_mapt reference_set;
    get_reference_set_sharing(lhs, reference_set, ns);

    forall_objects(it, reference_set.read())
    {
      const exprt &object=object_numbering[it->first];

      if(object.id()!="unknown")
        assign_rec(object, values_rhs, suffix, ns, recursion_set);
    }
  }
  else if(lhs.is_index())
  {
    if(lhs.operands().size()!=2)
      throw "index expected to have two operands";
      
    const typet &type=ns.follow(lhs.op0().type());
      
    assert(type.is_array() || type.is_incomplete_array() || type.id()=="#REF#");

    assign_rec(lhs.op0(), values_rhs, "[]"+suffix, ns, recursion_set);
  }
  else if(lhs.is_member())
  {
    if(lhs.operands().size()!=1)
      throw "member expected to have one operand";
    
    if(lhs.op0().is_nil()) return;
  
    const std::string &component_name=lhs.component_name().as_string();

    const typet &type=ns.follow(lhs.op0().type());

    assert(type.is_struct() ||
           type.id()=="union" ||
           type.id()=="incomplete_struct" ||
           type.id()=="incomplete_union");
           
    assign_rec(lhs.op0(), values_rhs, "."+component_name+suffix, 
               ns, recursion_set);
  }
  else if(lhs.id()=="valid_object" ||
		  lhs.id()=="deallocated_object" ||
          lhs.id()=="dynamic_size" ||
          lhs.id()=="dynamic_type")
  {
    // we ignore this here
  }
  else if(lhs.id()=="string-constant")
  {
    // someone writes into a string-constant
    // evil guy
  }
  else if(lhs.id()=="NULL-object")
  {
    // evil as well
  }
  else if(lhs.id()=="typecast")
  {
    const typecast_exprt &typecast_expr=to_typecast_expr(lhs);
  
    assign_rec(typecast_expr.op(), values_rhs, suffix, ns, recursion_set);
  }
  else if(lhs.id()=="zero_string" ||
          lhs.id()=="is_zero_string" ||
          lhs.id()=="zero_string_length")
  {
    // ignore
  }
  else if(lhs.id()=="byte_extract_little_endian" ||
          lhs.id()=="byte_extract_big_endian")
  {
    assert(lhs.operands().size()==2);
    assign_rec(lhs.op0(), values_rhs, suffix, ns, recursion_set);
  }
  else
    throw "assign NYI: `"+lhs.id_string()+"'";
}

/*******************************************************************\

Function: value_set_fit::do_function_call

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void value_set_fit::do_function_call(
  const irep_idt &function,
  const exprt::operandst &arguments,
  const namespacet &ns)
{
  const symbolt &symbol=ns.lookup(function);

  const code_typet &type=to_code_type(symbol.type);
  const code_typet::argumentst &argument_types=type.arguments();

  // these first need to be assigned to dummy, temporary arguments
  // and only thereafter to the actuals, in order
  // to avoid overwriting actuals that are needed for recursive
  // calls

  for(unsigned i=0; i<arguments.size(); i++)
  {
    const std::string identifier="value_set::" + function.as_string() + "::" +  
                                 "argument$"+i2string(i);
    add_var(identifier, "");
    exprt dummy_lhs=symbol_exprt(identifier, arguments[i].type());
    assign(dummy_lhs, arguments[i], ns);
  }

  // now assign to 'actual actuals'

  unsigned i=0;

  for(code_typet::argumentst::const_iterator
      it=argument_types.begin();
      it!=argument_types.end();
      it++)
  {
    const irep_idt &identifier=it->get_identifier();
    if(identifier=="") continue;

    add_var(identifier, "");
  
    const exprt v_expr=
      symbol_exprt("value_set::" + function.as_string() + "::" + 
                   "argument$"+i2string(i), it->type());
    
    exprt actual_lhs=symbol_exprt(identifier, it->type());
    assign(actual_lhs, v_expr, ns);
    i++;
  }
}

/*******************************************************************\

Function: value_set_fit::do_end_function

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void value_set_fit::do_end_function(
  const exprt &lhs,
  const namespacet &ns)
{
  if(lhs.is_nil()) return;

  std::string rvs = "value_set::return_value" + i2string(from_function);
  symbol_exprt rhs(rvs, lhs.type());

  assign(lhs, rhs, ns);
}

/*******************************************************************\

Function: value_set_fit::apply_code

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void value_set_fit::apply_code(
  const exprt &code,
  const namespacet &ns)
{
  const irep_idt &statement=code.statement();

  if(statement=="block")
  {
    forall_operands(it, code)
      apply_code(*it, ns);
  }
  else if(statement=="function_call")
  {
    // shouldn't be here
    assert(false);
  }
  else if(statement=="assign" ||
          statement=="init")
  {
    if(code.operands().size()!=2)
      throw "assignment expected to have two operands";

    assign(code.op0(), code.op1(), ns);
  }
  else if(statement=="decl")
  {
    if(code.operands().size()!=1)
      throw "decl expected to have one operand";

    const exprt &lhs=code.op0();

    if(!lhs.is_symbol())
      throw "decl expected to have symbol on lhs";

    assign(lhs, exprt("invalid", lhs.type()), ns);
  }
  else if(statement=="specc_notify" ||
          statement=="specc_wait")
  {
    // ignore, does not change variables
  }
  else if(statement=="expression")
  {
    // can be ignored, we don't expect sideeffects here
  }
  else if(statement=="cpp_delete" ||
          statement=="cpp_delete[]")
  {
    // does nothing
  }
  else if(statement=="free")
  {
    // this may kill a valid bit

    if(code.operands().size()!=1)
      throw "free expected to have one operand";

    do_free(code.op0(), ns);
  }
  else if(statement=="lock" || statement=="unlock")
  {
    // ignore for now
  }
  else if(statement=="nondet")
  {
    // doesn't do anything
  }
  else if(statement=="printf")
  {
    // doesn't do anything
  }
  else if(statement=="return")
  {
    // this is turned into an assignment
    if(code.operands().size()==1)
    {      
      std::string rvs = "value_set::return_value" + i2string(from_function);
      symbol_exprt lhs(rvs, code.op0().type());
      assign(lhs, code.op0(), ns);
    }
  }
  else
  {
    std::cerr << code.pretty() << std::endl;
    throw "value_set_fit: unexpected statement: "+id2string(statement);
  }
}
