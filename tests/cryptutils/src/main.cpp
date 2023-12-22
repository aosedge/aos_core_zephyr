/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#include <aos/common/tools/log.hpp>

#include "cryptutils/cryptutils.hpp"
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/oid.h>

const char* pemCertificate = "-----BEGIN CERTIFICATE-----\n"
                             "MIIDYTCCAkmgAwIBAgIUefLO+XArcR2jeqrgGqQlTM20N/swDQYJKoZIhvcNAQEL\n"
                             "BQAwQDELMAkGA1UEBhMCVUExEzARBgNVBAgMClNvbWUtU3RhdGUxDTALBgNVBAcM\n"
                             "BEt5aXYxDTALBgNVBAoMBEVQQU0wHhcNMjAwNzAzMTU0NzEzWhcNMjAwODAyMTU0\n"
                             "NzEzWjBAMQswCQYDVQQGEwJVQTETMBEGA1UECAwKU29tZS1TdGF0ZTENMAsGA1UE\n"
                             "BwwES3lpdjENMAsGA1UECgwERVBBTTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCC\n"
                             "AQoCggEBAMQAdRAyH+S6QSCYAq09Kn5uhNgBSEcOwUTdTH1W9BSgXaHAbHQmY0py\n"
                             "2EnXoQ4/B+xdFsFLRpW7dvDaXcgMjjX1B/Yn52lF2OLdTaRwcA5/5wU2hAKAs4lu\n"
                             "lLRS1Ez48cRutjyVwzB70EB78Og/79SbZnrE73RhE4gUGq1/7l95VrQeVyMxXPSz\n"
                             "T5DVQrwZ/TnNDHbB2WDP3oWi4EhHRSE3GxO9OvVIlWtps2/VLLGDjFKDDw57c+CJ\n"
                             "GtYDDSQGSAzYgKHLbC4YZdatLCzLOK+HYMBMQ+A+h1FFDOQiafjc2hhNAJJgK4YE\n"
                             "S2bTKPSDwUFvNXlojLUuRqmeJblTfU8CAwEAAaNTMFEwHQYDVR0OBBYEFGTTfDCg\n"
                             "4dwM/qAGCsMIt3akp3kaMB8GA1UdIwQYMBaAFGTTfDCg4dwM/qAGCsMIt3akp3ka\n"
                             "MA8GA1UdEwEB/wQFMAMBAf8wDQYJKoZIhvcNAQELBQADggEBACE7Dm84YTAhxnjf\n"
                             "BC5tekchab1LtG00kGD3olYIrk60ST9CTkeZKbIRUiJguu7YyS8/9acCI5PrL+Bj\n"
                             "JeThNo8BiHgEJ0MZkUI9JhIqWT1dHFZoiIWBJia6IyEEFrUEfKBpYW85Get25em3\n"
                             "xokm39qQ2HFKJXbzixE/4F792lUWU49g4tvClrkRrVISBxy1xPAQZ38dep9NMhHe\n"
                             "puBh64yKH073veYqAlkv4p+m0VDJsSRhrhHnC1n37P6UIy3FhyxfsnQ4JTbDsjyH\n"
                             "d43D/UeLrvqwwJvRWqwa1XCbkxyhBQ+/2Soq/ym+EFTgJJcT/UjXZMU6C3NF7oLa\n"
                             "2bbVjCU=\n"
                             "-----END CERTIFICATE-----";

const char* pemPrivKey = "-----BEGIN PRIVATE KEY-----\n"
                         "MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQDEqJBeJsLicPuq\n"
                         "VpXb+Atbiud0wlFb52vrcXYxDASGduN3E27oOa1L34rqkv4/2eJ9H6yH4F31vYfV\n"
                         "dF17Fjg0wv2JJKo/8RSUx2P0B9F5U1TUjqLK0TVYcdmYEJlz7CpphPaxkhYKQBcP\n"
                         "le3TSPMtJ5uI9XG63VDE7ZITXC6vfEgxOCmAl7pbS9NDF+tBZGB3eVQmkwKce6TE\n"
                         "BctskHfR/aQF5/8uKq8jHKZPFR9fve1awC7qohUzwU9mfimLZxOhFkUdbbjBvKGd\n"
                         "HGDClIN/bq1u/0gkIFoHa7JYJ8qiUDBMT9oeSk1gibLJtU77W+5rZvIBFX6gEgBA\n"
                         "EADafPOFAgMBAAECggEAN6eIq8yGbLUS0+MLKCRrem22ERoJElxM82W50CmZgkBV\n"
                         "IVbTlU71VzqdQsN0xrcv3L9BAXciwC/yKBt1dScVowDn5Y8Fy3C7pVtEU0R/KLnH\n"
                         "kRwIwCMy2KPns5xHAUJ9wj0J37j7Gc2HeTayBpBnmYjTAJCNrheCCyk8pNP6tliT\n"
                         "vHVUgIDcDgb/djBdYGgriLQtPSSdVnXQSWW3Hn40dO/U1ge/fzEzGBhErst5zH0W\n"
                         "Y+ZkH+IU5KZtMBXakpZrT+GsmT5YPTxVJ5dOhlafR0ZCiiDjHBvSklIhEM+TE0jc\n"
                         "jzkz7H/rdgwfRsLZhwpFYykV9mAziyXZ4vWBPLadJQKBgQDZ/8dHlQujxGZA9nIw\n"
                         "KKvZHVi5YrI1DwSl+aoyFVYPHdWJgzCrq4o23yNXu0PRy5G/+D2M/cbLkDSUdoR0\n"
                         "Ak7ktzQ3U/p6E4CnWG6uPzn55fXF1jNQm1EE7BhC+JNK741f5F015xfA/z1ywXCo\n"
                         "5UYqlaHE+oXKJziNzCw8a/7zTwKBgQDm8HTixzadIGm9dUV2Z0BEF8gL+JfLH02F\n"
                         "fSGbPh3PcxmnuopO35Qx9sUboWk/4w6V9/S2S1j3CHGOB6kQcsL+cqsuY+Ph8hEn\n"
                         "8VS8oquNHuHnqReIqCSjUzesHsRo6W9En8wPiUjhvKL0ZQx0Jjyu0j51if00FnGp\n"
                         "35nkZVRG6wKBgQCSxnkJBBvzJn/mS0f0jt2tb+nV39K2kKcDjQZ/dAgeY2rrjC3P\n"
                         "185WRYSJRCdUcKhwRRZEAHXBhxUvxGBHr2oo6gS5H8ysNsdPZOYYYa+KRr9kdWTV\n"
                         "Z81z7/Yh2TVqpcFdB+eOLEq9Ad0Aj6dnv/6vG1HwyvAbfK7CIe7Cu7/LVwKBgF4b\n"
                         "SmZHK7gnxy8PJLk1JfkZf8lxCdoZ7WsiLJmoXFl229N/rnCppygdGQFDazI/gmgW\n"
                         "XMAUQDKaXDu2X2x2d4Nckukah3hBPkB6lT8xQpsFJKVUQGTNr/BmLt+SwhLGXTMn\n"
                         "su350Zs7VWQl8Uc7dar/vbgD/QxGwRaqKXnq2Mb1AoGAD+LO5mL5e+BBtJgNRZXg\n"
                         "3IPhIjRlnbSHbRtahAPjnnvB6HBx6WQlOo2W0bAZZbvENj6kjbTG2vKnBlWk/HQP\n"
                         "HT3uTqxI6hSvkaDwlErUtqoznqX0AYNxMvBdYe+OtUenJbjcd1DCGeCUKDo8sdUz\n"
                         "QCIvlcR5eqknXQTkMZumhSM=\n"
                         "-----END PRIVATE KEY-----";

ZTEST_SUITE(cryptutils, NULL, NULL, NULL, NULL, NULL);

ZTEST(cryptutils, test_PEMToX509Certs)
{
    CryptUtils cryptUtils;

    size_t              pemLength = strlen(pemCertificate) + 1;
    aos::Array<uint8_t> pemBlob(reinterpret_cast<const uint8_t*>(pemCertificate), pemLength);
    printk("pemBlob size = %d\n", pemBlob.Size());

    aos::StaticArray<aos::crypto::x509::Certificate, 1> certs;

    auto error = cryptUtils.PEMToX509Certs(pemBlob, certs);
    zassert_equal(error, aos::ErrorEnum::eNone, "Failed to parse PEM blob");

    const auto& cert = certs[0];

    zassert_equal(cert.mSubjectKeyId == cert.mAuthorityKeyId, true, "cert.mSubjectKeyId != cert.mAuthorityKeyId");

    aos::StaticString<aos::crypto::cCertSubjSize> subject;
    zassert_equal(cryptUtils.DNToString(cert.mSubject, subject), aos::ErrorEnum::eNone, "Failed to convert DN");
    zassert_equal(subject == "C=UA, ST=Some-State, L=Kyiv, O=EPAM", true, "Unexpected subject");

    aos::StaticString<aos::crypto::cCertSubjSize> issuer;
    zassert_equal(cryptUtils.DNToString(cert.mIssuer, issuer), aos::ErrorEnum::eNone, "Failed to convert DN");
    zassert_equal(issuer == "C=UA, ST=Some-State, L=Kyiv, O=EPAM", true, "Unexpected issuer");

    zassert_equal(cert.mSubject == cert.mIssuer, true, "cert.mSubject != cert.mIssuer");

    aos::StaticArray<uint8_t, aos::crypto::cCertSubjSize> rawSubject;
    error = cryptUtils.CreateDN("C=UA, ST=Some-State, L=Kyiv, O=EPAM", rawSubject);
    zassert_equal(error, aos::ErrorEnum::eNone, "Failed to create DN");
}

ZTEST(cryptutils, test_CreateCSR)
{
    CryptUtils cryptUtils;

    aos::crypto::x509::CSR templ;
    const char*            subjectName = "CN=Test Subject,O=Organization,C=Country";
    zassert_equal(cryptUtils.CreateDN(subjectName, templ.mSubject), aos::ErrorEnum::eNone, "Failed to create DN");

    templ.mDNSNames.Resize(2);
    templ.mDNSNames[0] = "test1.com";
    templ.mDNSNames[1] = "test2.com";

    // OID for Subject Key Identifier
    const unsigned char subject_key_identifier_val[]
        = {0x64, 0xD3, 0x7C, 0x30, 0xA0, 0xE1, 0xDC, 0x0C, 0xFE, 0xA0, 0x06, 0x0A, 0xC3, 0x08, 0xB7, 0x76};

    size_t val_len = sizeof(subject_key_identifier_val);

    templ.mExtraExtensions.Resize(1);

    templ.mExtraExtensions[0].mId = MBEDTLS_OID_SUBJECT_KEY_IDENTIFIER;
    templ.mExtraExtensions[0].mValue = aos::Array<uint8_t>(subject_key_identifier_val, val_len);

    aos::StaticArray<uint8_t, 4096> pemCSR;

    aos::Array<uint8_t>     pemBlob(reinterpret_cast<const uint8_t*>(pemPrivKey), strlen(pemPrivKey) + 1);
    aos::crypto::PrivateKey privKey;
    privKey.mPublicKey = pemBlob;
    auto error = cryptUtils.CreateCSR(templ, privKey, pemCSR);
    zassert_equal(error, aos::ErrorEnum::eNone, "Failed to create CSR");
    zassert_equal(pemCSR.IsEmpty(), false, "Failed to create CSR");

    const char* csr = reinterpret_cast<const char*>(pemCSR.Get());
    printk("csr = %s\n", csr);

    const char* beginMarker = "-----BEGIN CERTIFICATE REQUEST-----";
    const char* endMarker = "-----END CERTIFICATE REQUEST-----";

    const char* begin = strstr(csr, beginMarker);
    const char* end = strstr(csr, endMarker);
    zassert_equal(begin != nullptr, true, "Unexpected CSR format");
    zassert_equal(end != nullptr, true, "Unexpected CSR format");
}

ZTEST(cryptutils, test_CreateCertificate)
{
    CryptUtils cryptUtils;

    aos::crypto::x509::Certificate templ;
    aos::crypto::x509::Certificate parent;

    uint8_t subjectID[] = {0x1, 0x2, 0x3, 0x4, 0x5};
    templ.mSubjectKeyId = aos::Array<uint8_t>(subjectID, sizeof(subjectID));

    uint8_t authorityID[] = {0x5, 0x4, 0x3, 0x2, 0x1};
    templ.mAuthorityKeyId = aos::Array<uint8_t>(authorityID, sizeof(authorityID));

    const char* subjectName = "CN=Test Subject,O=Organization,C=Country";
    zassert_equal(cryptUtils.CreateDN(subjectName, templ.mSubject), aos::ErrorEnum::eNone, "Failed to create DN");

    const char* issuer = "CN=Test Issuer,O=Organization,C=Country";
    zassert_equal(cryptUtils.CreateDN(issuer, templ.mIssuer), aos::ErrorEnum::eNone, "Failed to create DN");

    aos::Array<uint8_t>     pemBlob(reinterpret_cast<const uint8_t*>(pemPrivKey), strlen(pemPrivKey) + 1);
    aos::crypto::PrivateKey privKey;
    privKey.mPublicKey = pemBlob;

    aos::StaticArray<uint8_t, 4096> pemCRT;

    auto err = cryptUtils.CreateCertificate(templ, parent, privKey, pemCRT);
    zassert_equal(err, aos::ErrorEnum::eNone, "Failed to create certificate");
}
