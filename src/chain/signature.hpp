#ifndef GRUUT_ENTERPRISE_MERGER_SIGNATURE_HPP
#define GRUUT_ENTERPRISE_MERGER_SIGNATURE_HPP

#include "types.hpp"

namespace gruut {
class Signature {
public:
  Signature() = default;
  Signature(signer_id_type &&signer_id_, signature_type &&signer_signature_)
      : signer_id(signer_id_), signer_signature(signer_signature_) {}
  Signature(signer_id_type &signer_id_, signature_type &signer_signature_)
      : signer_id(signer_id_), signer_signature(signer_signature_) {}
  signer_id_type signer_id;
  signature_type signer_signature;
};
} // namespace gruut
#endif