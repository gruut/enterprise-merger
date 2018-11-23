#ifndef GRUUT_ENTERPRISE_MERGER_SIGNATURE_HPP
#define GRUUT_ENTERPRISE_MERGER_SIGNATURE_HPP

#include "types.hpp"

namespace gruut {
struct Signature {
  signer_id_type signer_id;
  timestamp sent_time;
  signature_type signer_signature;
};
} // namespace gruut
#endif
