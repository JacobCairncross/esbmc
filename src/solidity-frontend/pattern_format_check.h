#include <nlohmann/json.hpp>
#include <iostream>
#include <iostream>
#include <fstream>
#include <boost/optional.hpp>
#include <solidity-frontend/pattern_rule.h>
#include <boost/variant.hpp>
#include <boost/filesystem.hpp>

class pattern_format_checker
{
public:
  pattern_format_checker(
    const nlohmann::json &_ast_nodes);
  virtual ~pattern_format_checker() = default;

  bool do_pattern_check();
  boost::optional<nlohmann::json> check_AST_matches_pattern(nlohmann::json node, pattern_rule rule);
  bool literal_matches(nlohmann::json node, pattern_rule rule);
  pattern_rule* make_rule(nlohmann::json ruleObject);
  boost::variant<std::string, int, float> extract_value(nlohmann::json valueNode);


protected:
  const nlohmann::json &ast_nodes;
  bool recursiveCheck(nlohmann::json patternNode, nlohmann::json currentASTNode, int depth);
};