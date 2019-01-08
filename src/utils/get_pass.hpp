#ifndef GRUUT_ENTERPRISE_MERGER_GET_PASS_HPP
#define GRUUT_ENTERPRISE_MERGER_GET_PASS_HPP

#include <iostream>
#include <termio.h>
#include <zconf.h>

class getPass {
public:
  static int getch() {
    struct termios t_old, t_new;

    tcgetattr(STDIN_FILENO, &t_old);
    t_new = t_old;
    t_new.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &t_new);

    int ch = std::getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &t_old);
    return ch;
  }

  static std::string get(const std::string &prompt, bool show_asterisk = true) {
    const int BACKSPACE = 127;
    const int RETURN = 10;

    std::string password;
    unsigned char ch = 0;

    std::cout << prompt << std::endl;

    while ((ch = getch()) != RETURN) {
      if (ch == BACKSPACE) {
        if (password.length() != 0) {
          if (show_asterisk)
            std::cout << "\b \b";
          password.resize(password.length() - 1);
        }
      } else {
        password += ch;
        if (show_asterisk)
          std::cout << '*';
      }
    }
    std::cout << std::endl;
    return password;
  }
};

#endif // GRUUT_ENTERPRISE_MERGER_GET_PASS_HPP
