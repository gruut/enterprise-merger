#ifndef GRUUT_ENTERPRISE_MERGER_HTTP_CLIENT_HPP
#define GRUUT_ENTERPRISE_MERGER_HTTP_CLIENT_HPP

#include <memory>
#include <string>

#include "../../../include/curlpp.hpp"

namespace gruut {
class HttpClient {
public:
  HttpClient() = delete;
  HttpClient(const std::string &m_address);

  CURLcode post(const std::string &packed_msg);

  bool checkServStatus();

private:
  std::string getPostField(const std::string &key, const std::string &value);

  curlpp::Easy m_curl;
  std::string m_address;
};
} // namespace gruut
#endif
