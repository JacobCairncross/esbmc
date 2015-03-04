/*******************************************************************\

Module: C++ Language Module

Author: Daniel Kroening, kroening@cs.cmu.edu

\*******************************************************************/

#include <string.h>

#include <sstream>
#include <fstream>

#include <config.h>
#include <replace_symbol.h>

#include <ansi-c/c_preprocess.h>
#include <ansi-c/c_link.h>
#include <ansi-c/c_main.h>
#include <ansi-c/gcc_builtin_headers.h>

#include "cpp_language.h"
#include "expr2cpp.h"
#include "cpp_parser.h"
#include "cpp_typecheck.h"
#include "cpp_final.h"

/*******************************************************************\

Function: cpp_languaget::extensions

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

std::set<std::string> cpp_languaget::extensions() const
{
  std::set<std::string> s;

  s.insert("cpp");
  s.insert("cc");
  s.insert("ipp");
  s.insert("cxx");

  #ifndef _WIN32
  s.insert("C");
  #endif

  return s;
}

/*******************************************************************\

Function: cpp_languaget::modules_provided

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void cpp_languaget::modules_provided(std::set<std::string> &modules)
{
  modules.insert(parse_path);
}

/*******************************************************************\

Function: cpp_languaget::preprocess

  Inputs:

 Outputs:

 Purpose: ANSI-C preprocessing

\*******************************************************************/

bool cpp_languaget::preprocess(
  std::istream &instream,
  const std::string &path,
  std::ostream &outstream,
  message_handlert &message_handler)
{
  if(path=="")
    return c_preprocess(instream, "", outstream, true, message_handler);

  // check extension

  const char *ext=strrchr(path.c_str(), '.');
  if(ext!=NULL && std::string(ext)==".ipp")
  {
    std::ifstream infile(path.c_str());

    char ch;

    while(infile.read(&ch, 1))
      outstream << ch;

    return false;
  }

  return c_preprocess(instream, path, outstream, true, message_handler);
}

/*******************************************************************\

Function: cpp_languaget::internal_additions

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void cpp_languaget::internal_additions(std::ostream &out)
{
  out << "# 1 \"<built-in>\"" << std::endl;

  out << "void *operator new(unsigned int size);" << std::endl;

  // assume/assert
  out << "extern \"C\" void assert(bool assertion);" << std::endl;
  out << "extern \"C\" void __ESBMC_assume(bool assumption);" << std::endl;
  out << "extern \"C\" void __ESBMC_assert("
         "bool assertion, const char *description);" << std::endl;

  // __ESBMC_atomic_{begin,end}
  out << "extern \"C\" void __ESBMC_atomic_begin();" << std::endl;
  out << "extern \"C\" void __ESBMC_atomic_end();" << std::endl;

  // __CPROVER namespace
  out << "namespace __CPROVER { }" << std::endl;

  // for dynamic objects
  out << "unsigned __CPROVER::constant_infinity_uint;" << std::endl;
  out << "bool __ESBMC_alloc[__CPROVER::constant_infinity_uint];" << std::endl;
  out << "unsigned __ESBMC_alloc_size[__CPROVER::constant_infinity_uint];" << std::endl;
  out << " bool __ESBMC_deallocated[__CPROVER::constant_infinity_uint];" << std::endl;
  out << "bool __ESBMC_is_dynamic[__CPROVER::constant_infinity_uint];" << std::endl;

  // GCC stuff
  out << "extern \"C\" {" << std::endl;
  out << GCC_BUILTIN_HEADERS;

  // Forward decs for pthread main thread begin/end hooks. Because they're
  // pulled in from the C library, they need to be declared prior to pulling
  // them in, for type checking.
  out << "void pthread_start_main_hook(void);" << std::endl;
  out << "void pthread_end_main_hook(void);" << std::endl;

  //  Empty __FILE__ and __LINE__ definitions.
  out << "const char *__FILE__ = \"\";" << std::endl;
  out << "unsigned int __LINE__ = 0;" << std::endl;

  out << "}" << std::endl;
}

/*******************************************************************\

Function: cpp_languaget::parse

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

bool cpp_languaget::parse(
  std::istream &instream,
  const std::string &path,
  message_handlert &message_handler)
{
  // store the path

  parse_path=path;

  // preprocessing

  std::ostringstream o_preprocessed;

  internal_additions(o_preprocessed);

  if(preprocess(instream, path, o_preprocessed, message_handler))
    return true;

  std::istringstream i_preprocessed(o_preprocessed.str());

  // parsing

  cpp_parser.clear();
  cpp_parser.filename=path;
  cpp_parser.in=&i_preprocessed;
  cpp_parser.set_message_handler(&message_handler);
  cpp_parser.grammar=cpp_parsert::LANGUAGE;

  if(config.ansi_c.os==configt::ansi_ct::OS_WIN32)
    cpp_parser.mode=cpp_parsert::MSC;
  else
    cpp_parser.mode=cpp_parsert::GCC;

  cpp_scanner_init();

  bool result=cpp_parser.parse();

  // save result
  cpp_parse_tree.swap(cpp_parser.parse_tree);

  // save some memory
  cpp_parser.clear();

  return result;
}

/*******************************************************************\

Function: cpp_languaget::typecheck

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

bool cpp_languaget::typecheck(
  contextt &context,
  const std::string &module,
  message_handlert &message_handler)
{
  if(module=="") return false;

  contextt new_context;

  if(cpp_typecheck(cpp_parse_tree, new_context, module, message_handler))
    return true;

  return c_link(context, new_context, message_handler, module);
}

/*******************************************************************\

Function: cpp_languaget::final

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

bool cpp_languaget::final(
  contextt &context,
  message_handlert &message_handler)
{
  if(cpp_final(context, message_handler)) return true;
  if(c_main(context, "c::", "c::main", message_handler)) return true;

  return false;
}

/*******************************************************************\

Function: cpp_languaget::show_parse

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void cpp_languaget::show_parse(std::ostream &out)
{
  for(cpp_parse_treet::itemst::const_iterator it=
      cpp_parse_tree.items.begin();
      it!=cpp_parse_tree.items.end();
      it++)
    show_parse(out, *it);
}

/*******************************************************************\

Function: cpp_languaget::show_parse

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void cpp_languaget::show_parse(
  std::ostream &out,
  const cpp_itemt &item)
{
  if(item.is_linkage_spec())
  {
    const cpp_linkage_spect &linkage_spec=
      item.get_linkage_spec();

    for(cpp_linkage_spect::itemst::const_iterator
        it=linkage_spec.items().begin();
        it!=linkage_spec.items().end();
        it++)
      show_parse(out, *it);

    out << std::endl;
  }
  else if(item.is_namespace_spec())
  {
    const cpp_namespace_spect &namespace_spec=
      item.get_namespace_spec();

    out << "NAMESPACE " << namespace_spec.get_namespace()
        << ":" << std::endl;

    for(cpp_namespace_spect::itemst::const_iterator
        it=namespace_spec.items().begin();
        it!=namespace_spec.items().end();
        it++)
      show_parse(out, *it);

    out << std::endl;
  }
  else if(item.is_using())
  {
    const cpp_usingt &cpp_using=item.get_using();

    out << "USING ";
    if(cpp_using.get_namespace())
      out << "NAMESPACE ";
    out << cpp_using.name() << std::endl;
    out << std::endl;
  }
  else if(item.is_declaration())
  {
    item.get_declaration().output(out);
  }
  else
    out << "UNKNOWN: " << item << std::endl;
}

/*******************************************************************\

Function: new_cpp_language

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

languaget *new_cpp_language()
{
  return new cpp_languaget;
}

/*******************************************************************\

Function: cpp_languaget::from_expr

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

bool cpp_languaget::from_expr(
  const exprt &expr,
  std::string &code,
  const namespacet &ns)
{
  code=expr2cpp(expr, ns);
  return false;
}

/*******************************************************************\

Function: cpp_languaget::from_type

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

bool cpp_languaget::from_type(
  const typet &type,
  std::string &code,
  const namespacet &ns)
{
  code=type2cpp(type, ns);
  return false;
}

/*******************************************************************\

Function: cpp_languaget::to_expr

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

bool cpp_languaget::to_expr(
  const std::string &code,
  const std::string &module __attribute__((unused)),
  exprt &expr,
  message_handlert &message_handler,
  const namespacet &ns)
{
  expr.make_nil();

  // no preprocessing yet...

  std::istringstream i_preprocessed(code);

  // parsing

  cpp_parser.clear();
  cpp_parser.filename="";
  cpp_parser.in=&i_preprocessed;
  cpp_parser.set_message_handler(&message_handler);
  cpp_parser.grammar=cpp_parsert::EXPRESSION;
  cpp_scanner_init();

  bool result=cpp_parser.parse();

  if(cpp_parser.parse_tree.items.empty())
    result=true;
  else
  {
    // TODO
    //expr.swap(cpp_parser.parse_tree.declarations.front());

    // typecheck it
    result=cpp_typecheck(expr, message_handler, ns);
  }

  // save some memory
  cpp_parser.clear();

  return result;
}

/*******************************************************************\

Function: cpp_languaget::~cpp_languaget

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

cpp_languaget::~cpp_languaget()
{
}
