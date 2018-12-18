// TODO : 인증서 검증,

#pragma once

#include <botan-2/botan/auto_rng.h>
#include <botan-2/botan/data_src.h>
#include <botan-2/botan/ecdsa.h>
#include <botan-2/botan/pk_keys.h>
#include <botan-2/botan/pkcs8.h>
#include <botan-2/botan/pubkey.h>
#include <botan-2/botan/x509_obj.h>
#include <botan-2/botan/x509cert.h>
#include <iostream>

std::string ga_cert_pem = R"UPK(-----BEGIN CERTIFICATE-----
MIIDLDCCAhQCBgEZlK1CPjANBgkqhkiG9w0BAQsFADBVMQswCQYDVQQGEwJBVTET
MBEGA1UECAwKU29tZS1TdGF0ZTEhMB8GA1UECgwYSW50ZXJuZXQgV2lkZ2l0cyBQ
dHkgTHRkMQ4wDAYDVQQDDAVHcnV1dDAeFw0xODExMjQxNDEyMTRaFw0xODEyMjQx
NDEyMTRaMF4xCzAJBgNVBAYTAkFVMRMwEQYDVQQIDApTb21lLVN0YXRlMSEwHwYD
VQQKDBhJbnRlcm5ldCBXaWRnaXRzIFB0eSBMdGQxFzAVBgNVBAMMDjEyMDM4MTAy
MzgwMTIzMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA35Jf7Am6eBdy
zg5cmYHr+/tLvgKh8rIK0C9kJBFZ8a/se7XsDWjaF1Fxbm4YCCrY7pYAglBzOtJX
at1mi6TNgE9UGdyvo++R4sE2JSfCErCLEvtxPVV0f09LjOm2Z46Uc3AVXSdTVCas
OJxM3dda20/LlZT0xm7BtBpY7IspU/ZcqN4d2vaNbaZyCIQtzZV403eM6l92AhsA
cusOwlNLdw+7p/RlzjYs99vKyxLhz9mRPvsbnjJIurkRSjYX+C4jjNDvEJMOCCCH
UM2xy8dyYFpJFqqgcdjk6frWBMGbYRTvX4LNG4b2QOy/SAcvTOlQi/bKRLM+3XQG
hGXBXMn75QIDAQABMA0GCSqGSIb3DQEBCwUAA4IBAQCJs2hY8bMIWf4yw+5zbNIF
/aJqQRvu/FoeXkq8dcxxUj2c/s+rlxuoxhPUVnR3Q0fUwVxIN/23Ai3KFxk1nknO
7KkAsUkoCFJcZqYN2+rdIA/NJ6N1Hm8S7zXo5IexKmaNluMk3QoCBwraX+XgjGR6
FpgiTIvKlgMy97Mg/3rl3DyyC8MwsAF4Jpna16zYhkOKsOpB3/6zp10zvSNVaDhZ
dJ6MSWuZ1c6H/ConxqJJ4Ig274L9AYqV4KBslD9BN3+BSgPUOazCYkEkEgbNIBpr
IcLvsK86b2kKeMgNiI32t/M5rw53EzxKQyzg8vFqKtrj4z/UgtuK++G/PS575B9q
-----END CERTIFICATE-----)UPK";

namespace gruut {
class CertValidator {
private:
public:
  CertValidator() {}

  static bool valid(std::string &cert_pem_str) {}

  static bool caValid(std::string &ca_cert_pem) {
    try {
      Botan::DataSource_Memory cert_datasource(cert_pem_str);
      Botan::X509_Certificate ca_cert(cert_datasource);

      if (!ca_cert.is_CA_cert()) {
        return false;
      }

      Botan::ECDSA_PublicKey ca_pub_key = Botan::ECDSA_PublicKey pub_key(
          ca_cert.subject_public_key()->algorithm_identifier(),
          ca_cert.subject_public_key()->public_key_bits());

      if (!ca_cert.check_signature(ca_pub_key)) {
        return false;
      }

      return true;

    } catch (Botan::Exception &exception) {
      throw;
    }
  }

  static bool certValidTime(Botan::X509_Certificate &cert) {
    try {

      std::string start_time = "December 15, 2018";
      std::string end_time = "December 12, 2028";

      std::string cert_start_time =
          Botan::X509_Certificate::subject_info("X509.Certificate.start");
      std::string cert_end_time =
          Botan::X509_Certificate::subject_info("X509.Certificate.end");

      if (cert_start_time < start_time && cert_start_time > end_time) {
        return false;
      }
      if (cert_end_time > end_time && cert_end_time > start_time) {
        return false;
      }
      return true;

    } catch (Botan::Exception &exception) {
      throw;
    }
  }

  static bool test(Botan::X509_Certificate &cert) {
    try {
      // serial number, country,
      Botan::X509_Certificate::subject_info("X509.Certificate.serial");
    } catch (Botan::Exception &exception) {
      throw;
    }
  }

  static bool issuerAltName(Botan::X509_Certificate &cert) {
    try {
      Botan::X509_Certificate::issuer_alt_name();
      std::stirng issuer_alt_name = Botan::X509_Certificate::issuer_alt_name();
      if (issuer_alt_name != "Gruut Networks") {
        return false;
      }
      return true;

    } catch (Botan::Exception &exception) {
      throw;
    }
  }

  static bool subjectAltName(Botan::X509_Certificate &cert) {
    try {
      Botan::X509_Certificate::issuer_alt_name();
      std::string sub_alt_name = Botan::X509_Certificate::subject_alt_name();
      if (sub_alt_name != "gruut.net") {
        return false;
      }
      return true;

    } catch (Botan::Exception &exception) {
      throw;
    }
  }

  static Botan::X509_Certificate strToCert(std::string &cert_pem_str) {
    try {
      Botan::DataSource_Memory cert_datasource(cert_pem_str);
      Botan::X509_Certificate cert(cert_datasource);
      return cert;

    } catch (Botan::Exception &exception) {
      throw;
    }
  }
};

} // namespace gruut