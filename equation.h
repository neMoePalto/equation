#pragma once

#include <map>
#include <optional>
#include <string>


// Части уравнения b_bef + kx + b_aft = y
struct coefficients {
  std::string         b_before_kx;
  std::optional<char> operator_before_kx;
  std::string         kx;
  std::optional<char> operator_after_kx;
  std::string         b_after_kx;
  std::string         y;
};


class equation {
public:
  explicit equation(std::string s);
  double get_x();

private:
  coefficients parse(const std::map<char, std::size_t>& key_pos);
  void parse_expression_after_x(std::string expr, double &rhs_value);
  std::optional<double> calc_expression_before_x(std::string expr);
  void check_num_syntax(const std::string& value) const;

private:
  const std::string           valid_symbols_;
  const std::string           equation_str_;
  std::map<char, std::size_t> key_pos_;
};
