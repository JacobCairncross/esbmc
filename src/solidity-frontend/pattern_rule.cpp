#include "pattern_rule.h"

pattern_rule::pattern_rule(RuleType _ruleType){
  ruleType = _ruleType;
}

pattern_rule::pattern_rule(RuleType _ruleType, boost::variant<std::string, int, float, bool> _value){
  ruleType = _ruleType;
  value = _value;

}

pattern_rule::RuleType pattern_rule::rule_type(){
  return ruleType;
}
std::string pattern_rule::rule_name(){
  switch(ruleType){
  case root:
    return "ROOT";
  case key:
    return "KEY";
  case size:
    return "SIZE";
  case index:
    return "INDEX";
  case literalValue:
    return "LITERAL_VALUE";
  case contains:
    return "CONTAINS";
  case pass:
    return "PASS";
  }
}


boost::variant<std::string, int, float, bool> pattern_rule::get_value(){
  return value;
}

std::string pattern_rule::get_value_type_name(){
  switch(value.which()){
  case 0:
    return "string";
  case 1:
    return "int";
  case 2:
    return "float";
  case 3:
    return "bool";
  }
}


void pattern_rule::set_value(boost::variant<std::string, int, float, bool> _value){
  value = _value;
}
