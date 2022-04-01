#include <solidity-frontend/pattern_format_check.h>
#include <stdio.h>

pattern_format_checker::pattern_format_checker(const nlohmann::json &_ast_nodes):ast_nodes(_ast_nodes)
{}

bool pattern_format_checker::do_pattern_check()
{
  std::cout << "Commencing Pattern Checks" << std::endl;
  bool patternMatches = false;
  for(boost::filesystem::directory_entry& entry : boost::filesystem::directory_iterator("./savedPatterns"))
  {
//    std::ifstream ifs("./savedPatternSingleChild.json");
    std::ifstream ifs(entry.path().string());
    nlohmann::json patternNodes = nlohmann::json::parse(ifs);
    patternMatches = printValueOrKids(patternNodes, ast_nodes, 0)?true:patternMatches;
    if(patternMatches){
      std::cout << "Program matches pattern: " << entry.path().string() << std::endl;
    }
  }
  //Return true if at least one pattern has matched
  return patternMatches;
}

bool pattern_format_checker::printValueOrKids(nlohmann::json patternNode, nlohmann::json currentASTNode, int depth)
{
  std::string indent(4 * depth, ' ');
  //Extract rules object from pattern node
  pattern_rule* rule = make_rule(patternNode["ruleObject"]);
  if(rule->rule_type() == pattern_rule::pass){
    //In this scenario we know the result will match, by definition it passes the check. We just need to check if any
    //of the AST nodes children passes their checks. This could span out exponentially so pass should be used sparingly
    for(auto childNode : currentASTNode.items()){
      for(auto rule : patternNode["children"].items()){

        if(printValueOrKids(rule.value(), childNode.value(), depth+1)){
          return true;
        }
      }
    }
    return false;
  }

  boost::optional<nlohmann::json> matchResult = check_AST_matches_pattern(currentASTNode, *rule);
  if(matchResult)
  {
    //TODO: Possibly include groups in the pattern rules
    for(auto childRules : patternNode["children"]){
      if(!printValueOrKids(childRules, matchResult.get(), depth+1)){

        return false;
      }
    }

    return true;
  }
  return false;

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
      std::cout << "rule is size, but node is not array, is " << node.type_name() <<std::endl;
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
      std::cout << "rule is size, but node is not array" <<std::endl;

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

    std::cout << "Type name: " << node.type_name() << std::endl;
    switch(node.type()){
    case nlohmann::json::value_t::number_unsigned:
      if(node.get<int>() == boost::get<int>(rule.get_value())){
        return true;
      }
      else{
        return false;
      }
      break;
    case nlohmann::json::value_t::number_integer:
      if(node.get<int>() == boost::get<int>(rule.get_value())){
        return true;
      }
      else{
        return false;
      }
      break;
    case nlohmann::json::value_t::number_float:
      if(node.get<float>() == boost::get<float>(rule.get_value())){
        return true;
      }
      else{
        return false;
      }
      break;
    case nlohmann::json::value_t::string:
      if(node.get<std::string>() == boost::get<std::string>(rule.get_value())){
        return true;
      }
      else{
        return false;
      }
      break;
    case nlohmann::json::value_t::boolean:
      if(node.get<bool>() == boost::get<bool>(rule.get_value())){
        return true;
      }
      else{
        return false;
      }
      break;
    }
}

pattern_rule* pattern_format_checker::make_rule(nlohmann::json ruleObject){
  std::cout << ruleObject.dump() <<std::endl;
  //Default ruletype as pass
  pattern_rule::RuleType ruleType = pattern_rule::RuleType::pass;
  //TODO: make a value extractor function because this takes everything as strings. Make something that tries ints and floats before falling back on strings
  boost::variant<std::string, int, float, bool> value = extract_value(ruleObject["value"]);
  std::cout << "Value extracted" <<std::endl;
  std::string ruleString = to_string(ruleObject["rule"]);

  if(ruleString == "\"ROOT\""){
    ruleType = pattern_rule::RuleType::root;
  }
  else if(ruleString == "\"PASS\""){
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

boost::variant<std::string, int, float> pattern_format_checker::extract_value(nlohmann::json valueNode){

  switch(valueNode.type()){
  case nlohmann::json::value_t::number_float:
    return boost::variant<std::string, int, float, bool>(valueNode.get<float>());
  case nlohmann::json::value_t::string:
    return boost::variant<std::string, int, float, bool>(valueNode.get<std::string>());
  case nlohmann::json::value_t::number_integer:
    return boost::variant<std::string, int, float, bool>(valueNode.get<int>());
  case nlohmann::json::value_t::number_unsigned:
    return boost::variant<std::string, int, float, bool>(valueNode.get<int>());
  case nlohmann::json::value_t::boolean:
    return boost::variant<std::string, int, float, bool>(valueNode.get<bool>());
  default:
    std::cout << "Default value extracted" <<std::endl;
    return boost::variant<std::string, int, float, bool>();

  }
}

