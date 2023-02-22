#include "equation.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <stdexcept>


equation::equation(std::string s)
  : valid_symbols_{std::string{"1234567890"} + "+-*/" + '.' + "=x"} // TODO: Можно добавить X в верх. регистре
  , equation_str_{s}
  , key_pos_{{'x', std::string::npos},
             {'=', std::string::npos}} {
  // Проверка на общую корректность введенного уравнения:
  for (const auto& symb : equation_str_) {
    if (valid_symbols_.find(symb) == std::string::npos) {
      throw std::runtime_error("Error #1: String contains invalid symbol(-s)");
    }
  }

  // Поиск "ключевых символов" и проверка их количества:
  for (auto symb = equation_str_.cbegin(); symb != equation_str_.cend(); ++symb) {
    if (const auto it = key_pos_.find(*symb); it != key_pos_.cend()) {
      if (it->second == std::string::npos) {
        it->second = symb - equation_str_.cbegin();
      } else {
        throw std::runtime_error(std::string{"Error #2: String contains more than one symbol \""} + it->first + "\"");
      }
    }
  }

  for (const auto& [k, pos] : key_pos_) {
    if (pos == std::string::npos) {
      throw std::runtime_error(std::string{"Error #3: Symbol \""} + k + "\" " + "is missing");
    }
  }

  // Уравнение не может начинаться с символов '*' или '/':
  if (equation_str_.at(0) == '*' ||
      equation_str_.at(0) == '/') {
    throw std::runtime_error(std::string{"Error #4: Bad first symbol \""} + equation_str_.at(0) + "\"");
  }

  // Уравнение не может завершаться символами '*', '/', '+' или '-':
  if (char last_s = *(equation_str_.cend() - 1);
      last_s == '*' ||
      last_s == '/' ||
      last_s == '-' ||
      last_s == '+') {
    throw std::runtime_error(std::string{"Error #5: Bad last symbol \""} + last_s + "\"");
  }

  // Уравнение не может содержать недопустимые символы до и после знака '=':
  if (key_pos_.at('=') < equation_str_.size() - 1) {
    if (char symb_after_equal = equation_str_.at(key_pos_.at('=') + 1);
        symb_after_equal == '*' ||
        symb_after_equal == '/') {
      throw std::runtime_error(std::string{"Error #6: Bad symbol \""} + symb_after_equal + "\" after \"=\"");
    }
  } else {
    throw std::runtime_error(std::string{"Error #7: Expected a number after \"=\""});
  }

  if (char symb_before_equal = equation_str_.at(key_pos_.at('=') - 1);
      symb_before_equal == '*' ||
      symb_before_equal == '/' ||
      symb_before_equal == '-' ||
      symb_before_equal == '+') {
    throw std::runtime_error(std::string{"Error #8: Bad symbol \""} + symb_before_equal + "\" before \"=\"");
  }

  // Проверка символов до и после 'x':
  if (key_pos_.at('x') != 0) {
    char symb_before_x = equation_str_.at(key_pos_.at('x') - 1);
    const auto vs_cend = valid_symbols_.cbegin() + 10;
    const auto it = std::find(valid_symbols_.cbegin(), vs_cend, symb_before_x);
    if (it != vs_cend) {
      throw std::runtime_error(std::string{"Error #9: Bad symbol \""} + symb_before_x + "\" before \"x\"");
    }
  }

  if (key_pos_.at('x') < equation_str_.size() - 1) {
    char symb_after_x = equation_str_.at(key_pos_.at('x') + 1);
    const auto vs_cend = valid_symbols_.cbegin() + 10; // Охватывает все цифры
    const auto it = std::find(valid_symbols_.cbegin(), vs_cend, symb_after_x);
    if (it != vs_cend) {
      throw std::runtime_error(std::string{"Error #10: Bad symbol \""} + symb_after_x + "\" after \"x\"");
    }
  }

  // Проверка отсутствия дублирования операторов, например:
  //  2-*x=4   // Error
  std::optional<char> prev_symb;
  for (const auto& symb : equation_str_) {
    if (symb == '+' || symb == '-' || symb == '*' || symb == '/') {
      if (!prev_symb) {
        prev_symb = symb;
      } else {
        throw std::runtime_error(std::string{"Error #11: Bad symbol \""} + symb + "\"" +
                                 " after \"" + prev_symb.value() + "\"");
      }
    } else {
      prev_symb.reset();
    }
  }
}


double equation::get_x() {
  coefficients coefs = parse(key_pos_);

  {
    // Предполагается, что b_before_kx, b_after_kx и y - числа (например, -7),
    // а не последовательность вычислений (например, 7*4/2-9).
    std::vector<std::pair<std::string, std::string>> basic_values{{coefs.b_before_kx, "b_before_kx = "},
                                                                  {coefs.b_after_kx,  "b_after_kx = "},
                                                                  {coefs.y,           "y = "}};
    for (const auto& [val, str] : basic_values) {
      for (auto it = val.cbegin(); it != val.cend(); ++it) {
        if (*it == '*' || *it == '/' || *it == '-' || *it == '+') {
          if (it == val.cbegin() && (*it == '+' || *it == '-')) {
            continue;
          } else {
            throw std::runtime_error(std::string{"Error #13: Ожидается, что "} + str + val +
                                     " является числом, а не выражением");
          }
        }
      }
    }
  }

  check_num_syntax(coefs.b_before_kx);
  double b_bef = coefs.b_before_kx.empty() ? 0.0 : std::stod(coefs.b_before_kx);

  check_num_syntax(coefs.b_after_kx);
  double b_aft = coefs.b_after_kx.empty() ? 0.0
                                          : coefs.operator_after_kx == '-' ? std::stod(coefs.b_after_kx) * (- 1)
                                                                           : std::stod(coefs.b_after_kx);
  check_num_syntax(coefs.y);
  double rhs_value = std::stod(coefs.y) - b_bef - b_aft;

  // После предыдущих проверок предполагаем, что наше уравнение корректно
  auto x_pos = coefs.kx.find('x');
  assert(x_pos != std::string::npos);

  parse_expression_after_x(coefs.kx.substr(x_pos), rhs_value);
  auto expr_before_x = calc_expression_before_x(coefs.kx.substr(0, x_pos));

  double x = 0;

  if (expr_before_x.has_value()) {
    assert(x_pos > 0);
    const char op = coefs.kx.at(x_pos - 1);

    if (op == '*') {
      x = rhs_value / expr_before_x.value();
      if (coefs.operator_before_kx == '-') {
        x = x * -1;
      }
    } else if (op == '/') {
      if (rhs_value == 0) {
        throw std::runtime_error(std::string{"Error #16: Уравнение не имеет корней. В процессе решения уравнения возникает операция деления на 0"});
      }
      x = expr_before_x.value() / rhs_value;
      if (coefs.operator_before_kx == '-') {
        x = x * -1;
      }
    } else if (op == '-') {
      assert(expr_before_x == -1.0);
      x = rhs_value / expr_before_x.value();
    } else {
      assert(false);
    }
  } else {
    x = rhs_value;
  }

//  auto print = [](const std::string& s, bool wh_sp = true) {
//    std::cout << (s.empty() ? " " : s) << (wh_sp ? "__" : "");
//  };

//  std::cout << "Результат разбора: \n";
//  print(coefs.b_before_kx);
//  print({coefs.operator_before_kx.value_or(' ')});
//  print(coefs.kx);
//  print({coefs.operator_after_kx.value_or(' ')});
//  print(coefs.b_after_kx);
//  print(" = ");
//  print(coefs.y, false);
//  std::cout << std::endl;

  return x;
}


coefficients equation::parse(const std::map<char, std::size_t>& key_pos) {
  coefficients coefs;

  if (key_pos.at('x') < key_pos.at('=')) { // Уравнение вида kx + b = y
    coefs.y = equation_str_.substr(key_pos.at('=') + 1);
    assert(!coefs.y.empty()); // Уже проверяли в конструкторе

    auto left_kx_border = equation_str_.cbegin() + key_pos.at('x');
    for (; left_kx_border != equation_str_.cbegin(); --left_kx_border) {
      if (*left_kx_border == '+' || *left_kx_border == '-') {
        coefs.operator_before_kx = *left_kx_border;
        coefs.b_before_kx = equation_str_.substr(0, left_kx_border - equation_str_.cbegin());
        break;
      }
    }

    auto right_kx_border = equation_str_.cbegin() + key_pos.at('x');
    for (; right_kx_border != equation_str_.cbegin() + key_pos.at('='); ++right_kx_border) {
      if (*right_kx_border == '+' || *right_kx_border == '-') {
        coefs.operator_after_kx = *right_kx_border;
        coefs.b_after_kx = equation_str_.substr(right_kx_border - equation_str_.cbegin() + 1,
                                                equation_str_.cbegin() + key_pos.at('=') - right_kx_border - 1);
        assert(!coefs.b_after_kx.empty());
        break;
      }
    }

    std::size_t ident = left_kx_border == equation_str_.cbegin() ? 0 : 1;
    coefs.kx = equation_str_.substr(left_kx_border - equation_str_.cbegin() + ident,
                                    right_kx_border - left_kx_border - ident);
  } else {
    throw std::runtime_error(std::string{"Error #12: В уравнении \"x\" расположен после \"=\""});
  }

  return coefs;
}


void equation::parse_expression_after_x(std::string expr, double& rhs_value) {
  auto it_from = expr.crbegin();
  while (it_from != expr.crend()) {
    // Разбираем выражение с конца. Например, для x*10/20*4 у нас будет 3 итерации:
    // - делим rhs_value на 4;
    // - умножаем rhs_value на 20;
    // - делим rhs_value на 10.
    auto it = std::find_if(it_from, expr.crend(), [](const char& s) {
      return s == '*' || s == '/';
    });

    if (it != expr.crend()) {
      std::size_t num_len = it - it_from;
      assert(num_len > 0); // На основании прошлых проверок

      double num = 0;
      int pos = expr.crend() - it; // - it_from - num_len;
      auto num_str = expr.substr(pos, num_len);

      check_num_syntax(num_str);
      num = std::stod(num_str);

      if (*it == '*') {
        if (num == 0) {
          throw std::runtime_error(std::string{"Error #14.1: Уравнение не имеет корней, т.к. один из множителей \"x\" равен 0"});
        }
        rhs_value = rhs_value / num;

      } else if (*it == '/') {
        if (num == 0) {
          throw std::runtime_error(std::string{"Error #15.1: В уравнении присутствует операция деления на 0"});
        }
        rhs_value = rhs_value * num;

      } else {
        assert(false);
      }

      it_from = ++it;
    } else {
      break;
    }
  }
}


std::optional<double> equation::calc_expression_before_x(std::string expr) {
  if (expr.empty()) {
    return {};
  }

  if (expr == "-") {
    return -1.0;
  }

  std::optional<double> res;
  std::optional<std::string::const_iterator> op;

  auto it_from = expr.cbegin();
  while (it_from != expr.cend()) {
    // Разбираем выражение с начала. Например, для 10/20*4*x у нас будет 3 итерации:
    // - сохраняем 10;
    // - делим 10 на 20, сохраняем результат;
    // - умножаем rhs_value на 20;
    // - делим rhs_value на 10.
    auto it = std::find_if(it_from, expr.cend(), [](const char& s) {
      return s == '*' || s == '/';
    });

    if (it != expr.end()) {
      std::size_t num_len = it - it_from;
      assert(num_len > 0); // На основании прошлых проверок

      double num = 0;
      std::size_t pos = it_from - expr.cbegin();
      auto num_str = expr.substr(pos, num_len);

      check_num_syntax(num_str);
      num = std::stod(num_str);

      if (!res.has_value()) {
        res = num;
        assert(*it == '*' || *it == '/');
        op = it;
      } else {
        if (*op.value() == '*') {
          if (num == 0) {
            throw std::runtime_error(std::string{"Error #14.2: Уравнение не имеет корней, т.к. один из множителей \"x\" равен 0"});
          }
          res = res.value() * num;

        } else if (*op.value() == '/') {
          if (num == 0) {
            throw std::runtime_error(std::string{"Error #15.2: В уравнении присутствует операция деления на 0"});
          }
          res = res.value() / num;

        } else {
          assert(false);
        }

        op = it; // Оператор для следующего вычисления
      }

      it_from = ++it;
    }
  }

  return res;
}


void equation::check_num_syntax(const std::string &value) const {
  if (value.empty()) {
    return;
  }

  std::size_t amount = std::count(value.cbegin(), value.cend(), '.');
  if (amount > 1 ||
      value.back() == '.' ||
      value.front() == '.') {
    throw std::runtime_error(std::string{"Error #17: Последовательность символов "} + value +
                             " не является числом или содержит лишние символы '.'");
  }
}
