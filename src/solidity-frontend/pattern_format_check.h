#include <nlohmann/json.hpp>
#include <iostream>
#include <iostream>
#include <fstream>

class pattern_format_checker
{
public:
  pattern_format_checker(
    const nlohmann::json &_ast_nodes);
  virtual ~pattern_format_checker() = default;

  bool do_pattern_check();

protected:
  const nlohmann::json &ast_nodes;
  void printValueOrKids(nlohmann::json node, int depth);
};