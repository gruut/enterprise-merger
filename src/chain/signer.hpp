#ifndef GRUUT_ENTERPRISE_MERGER_SIGNER_HPP
#define GRUUT_ENTERPRISE_MERGER_SIGNER_HPP

#include <string>

namespace gruut {
using namespace std;
struct Signer {
  string cert;
  string address;

  bool isNew() { return true; }
};
} // namespace gruut

#endif
