#include <solidity-frontend/pattern_format_check.h>
#include <stdio.h>

pattern_format_checker::pattern_format_checker(const nlohmann::json &_ast_nodes):ast_nodes(_ast_nodes)
{}

bool pattern_format_checker::do_pattern_check()
{
  std::cout << "In pattern format checeker" <<std::endl;
  std::ifstream ifs("./savedPattern.json");
  nlohmann::json patternNodes = nlohmann::json::parse(ifs);
  printValueOrKids(patternNodes, 0);
}

void pattern_format_checker::printValueOrKids(nlohmann::json node, int depth)
{
  std::string indent(4 * depth, ' ');
  switch(node.type())
  {
  case nlohmann::json::value_t::object:
    for(auto &elem : node.items())
    {
      std::cout << indent << elem.key() << " |" << node.type_name() << std::endl;
      printValueOrKids(elem.value(), ++depth);
    }
    break;

  case nlohmann::json::value_t::array:
    for(auto &elem : node.items())
    {
      std::cout << indent << elem.key() << " |" << node.type_name() << std::endl;
      printValueOrKids(elem.value(), ++depth);
    }
    break;

  case nlohmann::json::value_t::string:
    std::cout << indent << "This is a string:  " << nlohmann::to_string(node)
              << std::endl;
    break;

  case nlohmann::json::value_t::boolean:
    std::cout << "This is a bool:  " << nlohmann::to_string(node) << std::endl;
    break;

  case nlohmann::json::value_t::number_float:
    std::cout << "This is a float:  " << nlohmann::to_string(node) << std::endl;
    break;

  case nlohmann::json::value_t::number_integer:
    std::cout << "This is a int:  " << nlohmann::to_string(node) << std::endl;
    break;

  case nlohmann::json::value_t::number_unsigned:
    std::cout << "This is a unsigned num:  " << nlohmann::to_string(node)
              << std::endl;
    break;

  case nlohmann::json::value_t::null:
    std::cout << "This is null:  " << nlohmann::to_string(node) << std::endl;
    break;

  default:
    std::cout << "This node has an unrecognized type" << std::endl;
    std::cout << node.type_name() << std::endl;
  }
}