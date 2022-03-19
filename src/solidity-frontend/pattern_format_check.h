#include <nlohmann/json.hpp>
#include <iostream>
#include <iostream>
#include <fstream>
#include <boost/optional.hpp>
#include <solidity-frontend/pattern_rule.h>

class pattern_format_checker
{
public:
  pattern_format_checker(
    const nlohmann::json &_ast_nodes);
  virtual ~pattern_format_checker() = default;

  bool do_pattern_check();
  boost::optional<nlohmann::json> check_AST_matches_pattern(nlohmann::json node, pattern_rule rule);
  bool literal_matches(nlohmann::json node, pattern_rule rule);


protected:
  const nlohmann::json &ast_nodes;
  bool printValueOrKids(nlohmann::json patternNode, nlohmann::json currentASTNode, int depth);
};