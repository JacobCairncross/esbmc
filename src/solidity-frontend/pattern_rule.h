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
  pattern_rule(RuleType _ruleType, boost::variant<std::string, int, float, bool> _value);

  RuleType rule_type();
  std::string rule_name();
  boost::variant<std::string, int, float, bool> get_value();
  std::string get_value_type_name();

  void set_value(boost::variant<std::string, int, float, bool> _value);

private:
  RuleType ruleType;

  boost::variant<std::string, int, float, bool> value;
};

#endif //ESBMC_PATTERN_RULE_H
