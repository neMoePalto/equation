#pragma once

#include <map>
#include <string>


// Коэффициенты уравнения b_bef + kx + b_aft = y
struct coefficients {
  std::string y;
  std::string b_before_kx;
  char operator_before_kx = 0x00; // Заменить char на enum'ы
  std::string kx;
  char operator_after_kx = 0x00; // Заменить char на enum'ы
  std::string b_after_kx;
};


class equation {
public:
  explicit equation(std::string s);
  double get_x();

private:
  coefficients parse(const std::map<char, std::size_t>& key_pos);
  void parse_expression_after_x(std::string expr, double &rhs_value);
  std::optional<double> calc_expression_before_x(std::string expr);

private:
  const std::string           valid_symbols_;
  const std::string           equation_str_;
  std::map<char, std::size_t> key_pos_;
};
