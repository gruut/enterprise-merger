#include "http_client.hpp"
#include <iostream>

using namespace std;

namespace gruut {
HttpClient::HttpClient(const string &m_address) : m_address(m_address) {}

CURLcode HttpClient::post(const string &packed_msg) {
  try {
    const string post_field = getPostField("blockraw", packed_msg);

    m_curl.setOpt(CURLOPT_URL, m_address.data());
    m_curl.setOpt(CURLOPT_POST, 1L);
    m_curl.setOpt(CURLOPT_POSTFIELDS, post_field.data());
    m_curl.setOpt(CURLOPT_POSTFIELDSIZE, post_field.size());

    m_curl.perform();
  } catch (curlpp::EasyException &err) {
    cout << "HttpClient post error: " << err.what() << endl;
    cout << "HttpClient Code: " << err.getErrorId() << endl;
    return err.getErrorId();
  }

  return CURLE_OK;
}

std::string HttpClient::getPostField(const string &key, const string &value) {
  const string post_field = key + "=" + value;
  return post_field;
}
} // namespace gruut
