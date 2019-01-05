#ifndef GRUUT_ENTERPRISE_MERGER_CERT_VALIDATOR_HPP
#define GRUUT_ENTERPRISE_MERGER_CERT_VALIDATOR_HPP

#include <botan-2/botan/asn1_alt_name.h>
#include <botan-2/botan/asn1_time.h>
#include <botan-2/botan/auto_rng.h>
#include <botan-2/botan/calendar.h>
#include <botan-2/botan/data_src.h>
#include <botan-2/botan/ecdsa.h>
#include <botan-2/botan/exceptn.h>
#include <botan-2/botan/hex.h>
#include <botan-2/botan/pk_keys.h>
#include <botan-2/botan/pkcs8.h>
#include <botan-2/botan/pubkey.h>
#include <botan-2/botan/rsa.h>
#include <botan-2/botan/x509_ext.h>
#include <botan-2/botan/x509_obj.h>
#include <botan-2/botan/x509cert.h>
#include <chrono>
#include <iostream>

class CertValidator {
public:
  CertValidator() {}

  static bool caCertValid(std::string &ca_cert_pem) {
    try {
      Botan::X509_Certificate ca_cert = strToCert(ca_cert_pem);

      if (!ca_cert.is_CA_cert()) {
        std::cout << "X509_CA: This certificate is not for a CA" << std::endl;
        return false;
      }

      Botan::RSA_PublicKey ca_pub_key(
          ca_cert.subject_public_key()->algorithm_identifier(),
          ca_cert.subject_public_key()->public_key_bits());

      if (!ca_cert.check_signature(ca_pub_key)) {
        return false;
      }
      return caCertValid(ca_cert);

    } catch (Botan::Exception &exception) {
      std::cout << "error on PEM to RSA PK: " << exception.what() << std::endl;
    }
    return true;
  }

  static bool caCertValid(Botan::X509_Certificate &ca_cert) {

    // TODO : cert 정보는 하드코딩 한 상태 -> 나중에 수정
    if (validTime(ca_cert) != true)
      return false;
    if (ca_cert.subject_info("Name")[0] != "//////////8=")
      return false;
    if (ca_cert.subject_info("RFC822")[0] != "contact@gruut.net")
      return false;
    if (ca_cert.subject_info("Organization")[0] != "Gruut Networks")
      return false;
    if (ca_cert.subject_info("Country")[0] != "KR")
      return false;
    if (ca_cert.issuer_info("Name")[0] != "//////////8=")
      return false;
    if (ca_cert.issuer_info("Organization")[0] != "Gruut Networks")
      return false;
    if (Botan::hex_encode(ca_cert.serial_number()) !=
        "BD9171B2F8905C20AB9CA974143C031C")
      return false;
    ;

    return true;
  }

  static bool certValid(std::string &ca_cert_pem, std::string &cert_pem_str) {
    try {
      Botan::X509_Certificate ca_cert = strToCert(ca_cert_pem);
      Botan::X509_Certificate cert = strToCert(cert_pem_str);

      Botan::RSA_PublicKey ca_pub_key(
          ca_cert.subject_public_key()->algorithm_identifier(),
          ca_cert.subject_public_key()->public_key_bits());

      if (!cert.check_signature(ca_pub_key)) {
        return false;
      }

      return certValid(cert);

    } catch (Botan::Exception &exception) {
      std::cout << "error on PEM to ECDSA PK: " << exception.what()
                << std::endl;
    }
    return true;
  }

  static bool certValid(Botan::X509_Certificate &cert) {

    // TODO : cert 정보는 하드코딩 한 상태 -> 나중에 수정
    if (validTime(cert) != true)
      return false;
    if (cert.subject_info("Name")[0] != "TUVSR0VSLTE=")
      return false;
    if (cert.subject_info("RFC822")[0] != "contact@gruut.net")
      return false;
    if (cert.subject_info("Organization")[0] != "Gruut Networks")
      return false;
    if (cert.subject_info("Country")[0] != "KR")
      return false;
    if (cert.issuer_info("Name")[0] != "//////////8=")
      return false;
    if (cert.issuer_info("Organization")[0] != "Gruut Networks")
      return false;
    if (Botan::hex_encode(cert.serial_number()) !=
        "EEE11C41D93BB0CAC68FA720914014C3")
      return false;

    return true;
  }

private:
  static Botan::X509_Certificate strToCert(std::string &cert_pem_str) {
    try {
      Botan::DataSource_Memory cert_datasource(cert_pem_str);
      Botan::X509_Certificate cert(cert_datasource);
      return cert;

    } catch (Botan::Exception &exception) {
      std::cout << "error on str to cert : " << exception.what() << std::endl;
    }
  }

  static bool validTime(Botan::X509_Certificate &cert) {
    Botan::X509_Time t1 = cert.not_before();
    Botan::X509_Time now = Botan::X509_Time(std::chrono::system_clock::now());
    Botan::X509_Time t2 = cert.not_after();

    if (t1.cmp(now) != -1 || t2.cmp(now) != 1)
      return false;

    return true;
  }
};

#endif