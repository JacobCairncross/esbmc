#include <solidity-frontend/pattern_format_check.h>
#include <stdio.h>

pattern_format_checker::pattern_format_checker(const nlohmann::json &_ast_nodes):ast_nodes(_ast_nodes)
{}

bool pattern_format_checker::do_pattern_check()
{
  std::cout << "In pattern format checeker" <<std::endl;
  std::ifstream ifs("./savedPattern.json");
  nlohmann::json patternNodes = nlohmann::json::parse(ifs);
  pattern_rule* patternRule = new pattern_rule(nlohmann::json::value_t::object, pattern_rule::RuleType::key, "nodeType");
  std::cout << "=========== Pattern rule ============" << std::endl;
  std::cout << patternRule->key_value() <<std::endl;
  bool patternMatches = printValueOrKids(patternNodes, ast_nodes, 0);
  std::cout << "PatternMatch: " << patternMatches << std::endl;
}

bool pattern_format_checker::printValueOrKids(nlohmann::json patternNode, nlohmann::json currentASTNode, int depth)
{
  std::string indent(4 * depth, ' ');
  switch(patternNode.type())
  {
  case nlohmann::json::value_t::object:
  {
    //Check all keys. If they match then check their value, children. Return true iff all keys return true (i.e match the pattern)
    for(auto &elem : patternNode.items())
    {
      std::cout << indent << elem.key() << " |" << patternNode.type_name()
                << std::endl;
      pattern_rule rule =
        pattern_rule(nlohmann::json::value_t::object, pattern_rule::key);
      rule.set_key_value(elem.key());
      boost::optional<nlohmann::json> matchResult = check_AST_matches_pattern(currentASTNode, rule);
      if(matchResult)
      {
        if(!printValueOrKids(elem.value(), matchResult.get(), ++depth)){
          return false;
        }
      }
      else
      {
        return false;
      }
    }
    return true;
    break;
  }

  case nlohmann::json::value_t::array:
  {
    for(auto &elem : patternNode.items())
    {
      std::cout << indent << elem.key() << " |" << patternNode.type_name()
                << std::endl;
      pattern_rule rule =
        pattern_rule(nlohmann::json::value_t::array, pattern_rule::index);
      if(depth == 4){
        rule.set_index_value(3);
      }
      else{
        rule.set_index_value(0);
      }
      boost::optional<nlohmann::json> matchResult =
        check_AST_matches_pattern(currentASTNode, rule);
      if(matchResult)
      {
        if(!printValueOrKids(elem.value(), matchResult.get(), ++depth))
        {
          return false;
        }
      }
      else
      {
        return false;
      }
    }
    return true;
    break;
  }
  case nlohmann::json::value_t::string:
  {
    std::cout << indent
              << "This is a string:  " << patternNode.get<std::string>()
              << std::endl;
    pattern_rule rule =
      pattern_rule(nlohmann::json::value_t::string, pattern_rule::literalValue);
    //TODO: Need to set value type for all other literls for the rest to work
    rule.set_value_type(nlohmann::json::value_t::string);
    rule.set_string_value(patternNode.get<std::string>());
    boost::optional<nlohmann::json> matchResult =
      check_AST_matches_pattern(currentASTNode, rule);
    if(matchResult)
    {
      return true;
    }
    else
    {
      return false;
    }
    break;
  }

  case nlohmann::json::value_t::boolean:
  {
    std::cout << indent << "This is a bool:  " << nlohmann::to_string(patternNode) << std::endl;
    pattern_rule rule = pattern_rule(nlohmann::json::value_t::boolean, pattern_rule::literalValue);
    rule.set_value_type(nlohmann::json::value_t::boolean);
    rule.set_bool_value(patternNode.get<bool>());
    boost::optional<nlohmann::json> matchResult = check_AST_matches_pattern(currentASTNode, rule);
    if(matchResult){
      return true;
    }
    else{
      return false;
    }
    break;
  }


  case nlohmann::json::value_t::number_float:
  {
    std::cout << indent << "This is a float:  " << nlohmann::to_string(patternNode)
              << std::endl;
    pattern_rule rule = pattern_rule(
      nlohmann::json::value_t::number_float, pattern_rule::literalValue);
    rule.set_value_type(nlohmann::json::value_t::number_float);
    rule.set_float_value(patternNode.get<float>());
    boost::optional<nlohmann::json> matchResult =
      check_AST_matches_pattern(currentASTNode, rule);
    if(matchResult)
    {
      return true;
    }
    else
    {
      return false;
    }
    break;
  }

  case nlohmann::json::value_t::number_integer:
  {
    std::cout << indent << "This is a int:  " << nlohmann::to_string(patternNode) << std::endl;
    pattern_rule rule = pattern_rule(
      nlohmann::json::value_t::number_integer, pattern_rule::literalValue);
    rule.set_value_type(nlohmann::json::value_t::number_integer);
    rule.set_int_value(patternNode.get<int>());
    boost::optional<nlohmann::json> matchResult =
      check_AST_matches_pattern(currentASTNode, rule);
    if(matchResult)
    {
      return true;
    }
    else
    {
      return false;
    }
    break;
  }

  case nlohmann::json::value_t::number_unsigned:
  {
    std::cout << indent << "This is a unsigned num:  " << nlohmann::to_string(patternNode)
              << std::endl;
    pattern_rule rule = pattern_rule(
      nlohmann::json::value_t::number_unsigned, pattern_rule::literalValue);
    rule.set_value_type(nlohmann::json::value_t::number_unsigned);
    rule.set_uint_value(patternNode.get<int>());
    boost::optional<nlohmann::json> matchResult =
      check_AST_matches_pattern(currentASTNode, rule);
    if(matchResult)
    {
      return true;
    }
    else
    {
      return false;
    }
    break;
  }

  case nlohmann::json::value_t::null:
    std::cout << indent << "This is null:  " << nlohmann::to_string(patternNode) << std::endl;
    break;

  default:
    std::cout << "This node has an unrecognized type" << std::endl;
    std::cout << patternNode.type_name() << std::endl;
  }
}

boost::optional<nlohmann::json> pattern_format_checker::check_AST_matches_pattern(nlohmann::json node, pattern_rule rule){
  //Could refactor this from switch to 3 ifs on node, is_object, is_array, is_primitive. Check it out once this works
  switch(rule.rule_type())
  {
  case pattern_rule::key:
    if(!node.is_object())
    {
      //If we're checking for the key of non-object something has gone wrong
      return boost::optional<nlohmann::json>{};
    }
    if(node.contains(rule.key_value()))
    {
      return boost::optional<nlohmann::json>{node[rule.key_value()]};
    }
    else
    {
      return boost::optional<nlohmann::json>{};
    }
    break;

  case pattern_rule::literalValue:
    if(literal_matches(node, rule))
    {
      //return given node to indicate true. Up to calling code to understand not to try and navigate further
      return boost::optional<nlohmann::json>{node};
    }
    else
    {
      return boost::optional<nlohmann::json>{};
    }
    break;

  case pattern_rule::size:
    if(!node.is_array())
    {
      //If we try to find the size of a non array something has went wrong
      return boost::optional<nlohmann::json>{};
    }
    if(node.size() == rule.size_value())
    {
      return node;
    }
    else
    {
      return boost::optional<nlohmann::json>{};
    }
  case pattern_rule::index:
    if(!node.is_array())
    {
      //If we try to find the size of a non array something has went wrong
      return boost::optional<nlohmann::json>{};
    }
    if(node.size() > rule.index_value())
    {
      return boost::optional<nlohmann::json>{node[rule.index_value()]};
    }
    else{
      return boost::optional<nlohmann::json>{};
    }

  }


}

bool pattern_format_checker::literal_matches(nlohmann::json node, pattern_rule rule){
  std::cout << "Literal type: " << node.type_name() << ", " << nlohmann::to_string(node) << std::endl;
  std::cout << "String val: " << node.get<std::string>() <<std::endl;
  if(rule.value_type() == nlohmann::json::value_t::number_unsigned){
    std::cout << "Rule int val: " << rule.int_value() <<std::endl;
    std::cout << "Rule string val: " << rule.string_value() <<std::endl;
  }
  if(node.type() == rule.value_type()){
    //Want another switch inside here? probs better to make another function so this doesn't get crowded
    //For each literal type convert node value to it and then compare with rule value
    //still confused about iterators with nlohmann though
    switch(node.type()){
    case nlohmann::json::value_t::number_unsigned:
      if(node.get<int>() == rule.uint_value()){
        std::cout << "uint matches" << std::endl;
        return true;
      }
      else{
        return false;
      }
      break;
    case nlohmann::json::value_t::number_integer:
      if(node.get<int>() == rule.int_value()){
        std::cout << "int matches" << std::endl;
        return true;
      }
      else{
        return false;
      }
      break;
    case nlohmann::json::value_t::number_float:
      if(node.get<float>() == rule.float_value()){
        std::cout << "float matches" << std::endl;
        return true;
      }
      else{
        return false;
      }
      break;
    case nlohmann::json::value_t::string:
      if(node.get<std::string>() == rule.string_value()){
        std::cout << "string matches" << std::endl;
        return true;
      }
      else{
        return false;
      }
      break;
    case nlohmann::json::value_t::boolean:
      if(node.get<bool>() == rule.bool_value()){
        std::cout << "bool matches" << std::endl;
        return true;
      }
      else{
        return false;
      }
      break;
    }
  }
}