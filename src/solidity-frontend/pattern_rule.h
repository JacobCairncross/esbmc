#ifndef ESBMC_PATTERN_RULE_H
#define ESBMC_PATTERN_RULE_H

#include <nlohmann/json.hpp>
#include <boost/variant.hpp>

class pattern_rule
{
public:
  enum RuleType{root, pass, key, literalValue, index, size, contains};
  //This constructor produces a useless rule as it doesn't have any value immediately so will need to use a setter to set the appropriate value
  pattern_rule(RuleType _ruleType);
  pattern_rule(RuleType _ruleType, boost::variant<std::string, int, float> _value);
  pattern_rule(nlohmann::json::value_t _nodeType, RuleType _ruleType);
  pattern_rule(nlohmann::json::value_t _nodeType, RuleType _ruleType, std::string _key);
  pattern_rule(nlohmann::json::value_t _nodeType, RuleType _ruleType, nlohmann::json::value_t _valueType, std::string _stringVal);
  //TODO: Add more constructors
  nlohmann::json::value_t node_type();
  RuleType rule_type();
  std::string rule_name();
  std::string key_value();
  //Value of literal can be string, signed int, unsigned int, float, bool
  nlohmann::json::value_t value_type();
  //Need to know value_type before calling these value functions so user knows which to call
  std::string string_value();
  //Unsure if we need int and uint value, nlohmann may not care as much as it seems
  int int_value();
  unsigned int uint_value();
  float float_value(); //Maybe change this to double?
  bool bool_value();
  int size_value();
  int index_value();
  boost::variant<std::string, int, float> get_value();


  //Setters, maybe return bool to indicate success or fail?
  void set_key_value(std::string value);
  void set_value_type(nlohmann::json::value_t value);
  void set_string_value(std::string value);
  void set_int_value(int value);
  void set_uint_value(unsigned int value);
  void set_float_value(float value);
  void set_bool_value(bool value);
  void set_size_value(int value);
  void set_index_value(int value);
  void set_value(boost::variant<std::string, int, float> _value);

private:
  nlohmann::json::value_t nodeType;
  RuleType ruleType;
  nlohmann::json::value_t valueType;
  std::string keyValue;
  std::string stringValue;
  int intValue;
  unsigned int uintValue;
  float floatValue;
  bool boolValue;
  int sizeValue;
  int indexValue;
  boost::variant<std::string, int, float> value;
};

#endif //ESBMC_PATTERN_RULE_H
