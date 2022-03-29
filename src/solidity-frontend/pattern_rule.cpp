#include "pattern_rule.h"

pattern_rule::pattern_rule(RuleType _ruleType){
  ruleType = _ruleType;
}

pattern_rule::pattern_rule(RuleType _ruleType, boost::variant<std::string, int, float> _value){
  ruleType = _ruleType;
  value = _value;

}


pattern_rule::pattern_rule(nlohmann::json::value_t _nodeType, RuleType _ruleType){
  nodeType = _nodeType;
  ruleType = _ruleType;
}
pattern_rule::pattern_rule(nlohmann::json::value_t _nodeType, RuleType _ruleType, std::string _key){
  nodeType = _nodeType;
  ruleType = _ruleType;
  keyValue = _key;
}
pattern_rule::pattern_rule(nlohmann::json::value_t _nodeType, RuleType _ruleType, nlohmann::json::value_t _valueType, std::string _stringVal){
  nodeType = _nodeType;
  ruleType = _ruleType;
  valueType = _valueType;
  stringValue = _stringVal;
}

nlohmann::json::value_t pattern_rule::node_type(){
  return nodeType;
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

std::string pattern_rule::key_value(){
  return keyValue;
}
//Value of literal can be string, signed int, unsigned int, float, bool
nlohmann::json::value_t pattern_rule::value_type(){
  return valueType;
}
//Need to know value_type before calling these value functions so user knows which to call
std::string pattern_rule::string_value(){
  return stringValue;
}
int pattern_rule::int_value(){
  return intValue;
}
unsigned int pattern_rule::uint_value(){
  return uintValue;
}
float pattern_rule::float_value(){
  return floatValue;
}
bool pattern_rule::bool_value(){
  return boolValue;
}
int pattern_rule::size_value(){
  return sizeValue;
}
int pattern_rule::index_value(){
  return indexValue;
}

boost::variant<std::string, int, float> pattern_rule::get_value(){
  return value;
}

void pattern_rule::set_key_value(std::string value){
  keyValue = value;
}
void pattern_rule::set_value_type(nlohmann::json::value_t value){
  valueType = value;
}
void pattern_rule::set_string_value(std::string value){
  stringValue = value;
}
void pattern_rule::set_int_value(int value){
  intValue = value;
}
void pattern_rule::set_uint_value(unsigned int value){
  uintValue = value;
}
void pattern_rule::set_float_value(float value){
  floatValue = value;
}
void pattern_rule::set_bool_value(bool value){
  boolValue = value;
}
void pattern_rule::set_size_value(int value){
  sizeValue = value;
}
void pattern_rule::set_index_value(int value){
  indexValue = value;
}

void pattern_rule::set_value(boost::variant<std::string, int, float> _value){
  value = _value;
}
