#include "http_client.hpp"
#include <iostream>

using namespace std;

namespace gruut {
HttpClient::HttpClient(const string &m_address) : m_address(m_address) {}

CURLcode HttpClient::post(const string &packed_msg) {
  CLOG(INFO, "HTTP") << "POST (" << m_address << ")";
  try {
    m_curl.setOpt(CURLOPT_URL, m_address.data());
    m_curl.setOpt(CURLOPT_POST, 1L);

    const auto escaped_field_data = m_curl.escape(packed_msg);
    const string post_field = getPostField("message", escaped_field_data);
    m_curl.setOpt(CURLOPT_POSTFIELDS, post_field.data());
    m_curl.setOpt(CURLOPT_POSTFIELDSIZE, post_field.size());

    m_curl.perform();
  } catch (curlpp::EasyException &err) {
    CLOG(ERROR, "HTTP") << err.what();
    return err.getErrorId();
  }

  return CURLE_OK;
}

bool HttpClient::checkServStatus() {
  try {
    m_curl.setOpt(CURLOPT_URL, m_address.data());
    m_curl.setOpt(CURLOPT_FAILONERROR, 1L);

    m_curl.perform();
  } catch (curlpp::EasyException &err) {
    return false;
  }
  return true;
}

std::string HttpClient::getPostField(const string &key, const string &value) {
  const string post_field = key + "=" + value;
  return post_field;
}
} // namespace gruut
