#include "http_client.hpp"
#include <iostream>

using namespace std;

namespace gruut {
HttpClient::HttpClient(const string &m_address) : m_address(m_address) {
  el::Loggers::getLogger("HTTP");
}

CURLcode HttpClient::post(const string &packed_msg) {
  try {
    m_curl.setOpt(CURLOPT_URL, m_address.data());
    m_curl.setOpt(CURLOPT_POST, 1L);

    const auto escaped_field_data = m_curl.escape(packed_msg);
    const string post_field = getPostField("message", escaped_field_data);
    CLOG(INFO, "HTTP") << "POST (" << m_address << ", " << post_field.size()
                       << "bytes )";

    m_curl.setOpt(CURLOPT_POSTFIELDS, post_field.data());
    m_curl.setOpt(CURLOPT_POSTFIELDSIZE, post_field.size());

    m_curl.perform();
  } catch (curlpp::EasyException &err) {
    CLOG(ERROR, "HTTP") << err.what();
    return err.getErrorId();
  }

  return CURLE_OK;
}

CURLcode HttpClient::postAndGetReply(const string &msg, json &response_json) {
  try {
    m_curl.setOpt(CURLOPT_URL, m_address.data());
    m_curl.setOpt(CURLOPT_POST, 1L);

    const auto escaped_filed_data = m_curl.escape(msg);
    const string post_field = getPostField("message", escaped_filed_data);
    CLOG(INFO, "HTTP") << "POST (" << m_address << ", " << post_field.size()
                       << "bytes )";

    m_curl.setOpt(CURLOPT_POSTFIELDS, post_field.data());
    m_curl.setOpt(CURLOPT_POSTFIELDSIZE, post_field.size());

    string http_data;

    m_curl.setOpt(CURLOPT_WRITEFUNCTION, writeCallback);
    m_curl.setOpt(CURLOPT_WRITEDATA, &http_data);
    m_curl.perform();

    response_json = json::parse(http_data);

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

size_t HttpClient::writeCallback(const char *in, size_t size, size_t num,
                                 string *out) {
  const std::size_t total_bytes(size * num);
  out->append(in, total_bytes);
  return total_bytes;
}

} // namespace gruut
