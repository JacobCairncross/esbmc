#include <solidity-frontend/pattern_format_check.h>
#include <stdio.h>

pattern_format_checker::pattern_format_checker(const nlohmann::json &_ast_nodes):ast_nodes(_ast_nodes)
{}

bool pattern_format_checker::do_pattern_check()
{
  std::cout << "In pattern format checeker" <<std::endl;
  std::ifstream ifs("./savedPatternItem.json");
  nlohmann::json patternNodes = nlohmann::json::parse(ifs);
  std::cout << "=========== Pattern rule ============" << std::endl;
  bool patternMatches = printValueOrKids(patternNodes, ast_nodes, 0);
  std::cout << "PatternMatch: " << patternMatches << std::endl;
  return patternMatches;
}

bool pattern_format_checker::printValueOrKids(nlohmann::json patternNode, nlohmann::json currentASTNode, int depth)
{
  std::string indent(4 * depth, ' ');
  //Extract rules object from pattern node
  pattern_rule* rule = make_rule(patternNode["ruleObject"]);
//  std::cout << indent << "Trying rule: " << rule->rule_name() << " with value: " << boost::get<std::string>(rule->get_value()) << std::endl;
//    std::cout << indent << "Rule name: " << rule->rule_name() << " with value: " << boost::get<std::string>(rule->get_value()) << std::endl;

  if(rule->rule_type() == pattern_rule::pass){
    //In this scenario we know the result will match, by definition it passes the check. We just need to check if any
    //of the AST nodes children passes their checks. This could span out exponentially so pass should be used sparingly
    for(auto childNode : currentASTNode.items()){
      for(auto rule : patternNode["children"].items()){
        std::cout <<indent<< "Checking node " << childNode.key() << ", with rule " << rule.value()["ruleObject"]["rule"] << std::endl;

        if(printValueOrKids(rule.value(), childNode.value(), depth+1)){
          std::cout << indent << "This is true " << std::endl;
          return true;
        }
      }
    }
    std::cout << indent << "This is false " << std::endl;
    return false;
  }
  boost::optional<nlohmann::json> matchResult = check_AST_matches_pattern(currentASTNode, *rule);
  if(matchResult)
  {
    //TODO: Possibly include groups in the pattern rules
    for(auto childRules : patternNode["children"]){
      if(!printValueOrKids(childRules, matchResult.get(), depth+1)){
        std::cout << indent << "This is false " << std::endl;

        return false;
      }
    }
    std::cout << indent << "This is true " << std::endl;

    return true;
  }
  else{
    std::cout<< indent  << "This is false " << std::endl;

    return false;
  }

}

boost::optional<nlohmann::json> pattern_format_checker::check_AST_matches_pattern(nlohmann::json node, pattern_rule rule){
  //Could refactor this from switch to 3 ifs on node, is_object, is_array, is_primitive. Check it out once this works
  switch(rule.rule_type())
  {
  case pattern_rule::root:
    return boost::optional<nlohmann::json>{node};
  case pattern_rule::key:
    if(!node.is_object())
    {
      //If we're checking for the key of non-object something has gone wrong
      return boost::optional<nlohmann::json>{};
    }
    for(auto item : node.items()){
      std::cout << "Key in obj: " << item.key() << ", ";
    }
    std::cout << std::endl;
    if(node.contains(boost::get<std::string>(rule.get_value())))
    {
      return boost::optional<nlohmann::json>{node[boost::get<std::string>(rule.get_value())]};
    }
    else
    {
      std::cout << "Node did not contain key: " << boost::get<std::string>(rule.get_value()) << std::endl;
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
      std::cout << "Node did not match literal value of: " << boost::get<std::string>(rule.get_value()) << std::endl;
      return boost::optional<nlohmann::json>{};
    }
    break;

  case pattern_rule::size:
    if(!node.is_array())
    {
      //If we try to find the size of a non array something has went wrong
      return boost::optional<nlohmann::json>{};
    }
    if(node.size() == boost::get<int>(rule.get_value()))
    {
      return node;
    }
    else
    {
      std::cout << "Array was not of size: " << boost::get<int>(rule.get_value()) << std::endl;
      return boost::optional<nlohmann::json>{};
    }
  case pattern_rule::index:
    if(!node.is_array())
    {
      //If we try to find the size of a non array something has went wrong
      return boost::optional<nlohmann::json>{};
    }
    int value = boost::get<int>(rule.get_value());
    if(node.size() > value)
    {
      return boost::optional<nlohmann::json>{node[value]};
    }
    else{
      std::cout << "Array is of size: " << node.size() << " but rule requested index of " << boost::get<int>(rule.get_value()) << std::endl;
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

pattern_rule* pattern_format_checker::make_rule(nlohmann::json ruleObject){
  std::cout << ruleObject.dump() << std::endl;
  pattern_rule::RuleType ruleType;
  //TODO: make a value extractor function because this takes everything as strings. Make something that tries ints and floats before falling back on strings
  boost::variant<std::string, int, float> value = ruleObject["value"].get<std::string>();
  std::string ruleString = to_string(ruleObject["rule"]);

  if(ruleString == "\"PASS\""){
    ruleType = pattern_rule::RuleType::pass;
  }
  else if(ruleString == "\"KEY\""){
    ruleType = pattern_rule::RuleType::key;
  }
  else if(ruleString == "\"SIZE\""){
    ruleType = pattern_rule::RuleType::size;

  }
  else if(ruleString == "\"INDEX\""){
    ruleType = pattern_rule::RuleType::index;
  }
  else if(ruleString == "\"LITERAL_VALUE\""){
    ruleType = pattern_rule::RuleType::literalValue;
  }
  pattern_rule* rule = new pattern_rule(ruleType);
  rule->set_value(value);
  return rule;
}

