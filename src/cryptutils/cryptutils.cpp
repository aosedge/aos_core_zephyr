#include <mbedtls/oid.h>

#include "cryptutils.hpp"

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

aos::Error CryptUtils::CreateCertificate(const aos::crypto::x509::Certificate& templ,
    const aos::crypto::x509::Certificate& parent, const aos::crypto::PrivateKey& privKey, aos::Array<uint8_t>& pemCert)
{
    mbedtls_x509write_cert   cert;
    mbedtls_pk_context       pk;
    mbedtls_entropy_context  entropy;
    mbedtls_ctr_drbg_context ctrDrbg;

    auto err = InitializeCertificate(cert, pk, ctrDrbg, entropy);
    if (err != aos::ErrorEnum::eNone) {
        return err;
    }

    auto cleanUpContext = [&]() {
        mbedtls_x509write_crt_free(&cert);
        mbedtls_pk_free(&pk);
        mbedtls_ctr_drbg_free(&ctrDrbg);
        mbedtls_entropy_free(&entropy);
    };

    err = ParsePrivateKey(pk, privKey, ctrDrbg);
    if (err != aos::ErrorEnum::eNone) {
        cleanUpContext();

        return err;
    }

    err = SetCertificateProperties(cert, pk, ctrDrbg, templ, parent);
    if (err != aos::ErrorEnum::eNone) {
        cleanUpContext();

        return err;
    }

    return WriteCertificatePem(cert, ctrDrbg, pemCert);
}

aos::Error CryptUtils::PEMToX509Certs(
    const aos::Array<uint8_t>& pemBlob, aos::Array<aos::crypto::x509::Certificate>& resultCerts)
{
    mbedtls_x509_crt crt;
    mbedtls_x509_crt_init(&crt);

    int ret = mbedtls_x509_crt_parse(&crt, pemBlob.Get(), pemBlob.Size());
    if (ret != 0) {
        mbedtls_x509_crt_free(&crt);

        return AOS_ERROR_WRAP(ret);
    }

    mbedtls_x509_crt* currentCrt = &crt;

    while (currentCrt != nullptr) {
        aos::crypto::x509::Certificate cert;

        auto err = ParseX509Certs(currentCrt, cert);
        if (err != aos::ErrorEnum::eNone) {
            mbedtls_x509_crt_free(&crt);

            return err;
        }

        resultCerts.PushBack(cert);

        currentCrt = currentCrt->next;
    }

    mbedtls_x509_crt_free(&crt);

    return aos::ErrorEnum::eNone;
}

aos::Error CryptUtils::CreateCSR(
    const aos::crypto::x509::CSR& templ, const aos::crypto::PrivateKey& privKey, aos::Array<uint8_t>& pemCSR)
{
    mbedtls_x509write_csr    csr;
    mbedtls_pk_context       key;
    mbedtls_entropy_context  entropy;
    mbedtls_ctr_drbg_context ctrDrbg;

    auto err = InitializeCSR(csr, key, ctrDrbg, entropy);
    if (err != aos::ErrorEnum::eNone) {
        return err;
    }

    auto cleanUpContext = [&]() {
        mbedtls_x509write_csr_free(&csr);
        mbedtls_pk_free(&key);
        mbedtls_ctr_drbg_free(&ctrDrbg);
        mbedtls_entropy_free(&entropy);
    };

    err = ParsePrivateKey(key, privKey, ctrDrbg);
    if (err != aos::ErrorEnum::eNone) {
        cleanUpContext();

        return err;
    }

    err = SetCSRProperties(csr, key, templ);
    if (err != aos::ErrorEnum::eNone) {
        cleanUpContext();

        return err;
    }

    err = WriteCSRPem(csr, ctrDrbg, pemCSR);
    if (err != aos::ErrorEnum::eNone) {
        cleanUpContext();

        return err;
    }

    cleanUpContext();

    return aos::ErrorEnum::eNone;
}

aos::Error CryptUtils::CreateDN(const aos::String& commonName, aos::Array<uint8_t>& result)
{
    mbedtls_asn1_named_data* dn {};
    int                      ret = mbedtls_x509_string_to_names(&dn, commonName.CStr());
    if (ret != 0) {
        return AOS_ERROR_WRAP(ret);
    }

    unsigned char  buf[1024];
    unsigned char* c = buf + sizeof(buf);

    ret = mbedtls_x509_write_names(&c, buf, dn);
    if (ret < 0) {
        return AOS_ERROR_WRAP(ret);
    }

    size_t len = buf + sizeof(buf) - c;
    result = aos::Array<uint8_t>(c, len);

    return aos::ErrorEnum::eNone;
}

aos::Error CryptUtils::DNToString(const aos::Array<uint8_t>& dn, aos::String& result)
{
    int                  ret {};
    size_t               len;
    const unsigned char* end = dn.Get() + dn.Size();
    unsigned char*       currentPos = const_cast<aos::Array<uint8_t>&>(dn).Get();

    if ((ret = mbedtls_asn1_get_tag(&currentPos, end, &len, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE)) != 0) {
        return AOS_ERROR_WRAP(ret);
    }

    const unsigned char* sequenceEnd = currentPos + len;

    while (currentPos < sequenceEnd) {
        if ((ret = mbedtls_asn1_get_tag(&currentPos, sequenceEnd, &len, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SET))
            != 0) {
            return AOS_ERROR_WRAP(ret);
        }

        const unsigned char* setEnd = currentPos + len;

        if ((ret = mbedtls_asn1_get_tag(&currentPos, setEnd, &len, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE))
            != 0) {
            return AOS_ERROR_WRAP(ret);
        }

        const unsigned char* seqEnd = currentPos + len;

        if ((ret = mbedtls_asn1_get_tag(&currentPos, seqEnd, &len, MBEDTLS_ASN1_OID)) != 0) {
            return AOS_ERROR_WRAP(ret);
        }

        auto oid {aos::Array<uint8_t> {currentPos, len}};

        auto err = GetOIDString(oid, result);
        if (err != aos::ErrorEnum::eNone) {
            return err;
        }

        currentPos += len;

        unsigned char tag = *currentPos;

        if (tag != MBEDTLS_ASN1_UTF8_STRING && tag != MBEDTLS_ASN1_PRINTABLE_STRING) {
            return aos::ErrorEnum::eInvalidArgument;
        }

        if ((ret = mbedtls_asn1_get_tag(&currentPos, seqEnd, &len, tag)) != 0) {
            return AOS_ERROR_WRAP(ret);
        }

        result.Insert(
            result.end(), reinterpret_cast<const char*>(currentPos), reinterpret_cast<const char*>(currentPos) + len);
        result.Append(", ");

        currentPos += len;
    }

    if (!result.IsEmpty()) {
        // Remove the last two characters (", ")
        result.Resize(result.Size() - 2);
    }

    return aos::ErrorEnum::eNone;
}

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

aos::Error CryptUtils::GetOIDString(aos::Array<uint8_t>& oid, aos::String& result)
{
    mbedtls_asn1_buf oidBuf;
    oidBuf.p = oid.Get();
    oidBuf.len = oid.Size();

    const char* shortName {};
    int         ret = mbedtls_oid_get_attr_short_name(&oidBuf, &shortName);
    if (ret != 0) {
        return AOS_ERROR_WRAP(ret);
    }

    if (shortName == nullptr) {
        return aos::ErrorEnum::eNone;
    }

    result.Append(shortName).Append("=");

    return aos::ErrorEnum::eNone;
}

aos::Error CryptUtils::ParseX509Certs(mbedtls_x509_crt* currentCrt, aos::crypto::x509::Certificate& cert)
{
    auto err = GetX509CertSubject(cert, currentCrt);
    if (err != aos::ErrorEnum::eNone) {
        return err;
    }

    err = GetX509CertIssuer(cert, currentCrt);
    if (err != aos::ErrorEnum::eNone) {
        return err;
    }

    err = GetX509CertSerialNumber(cert, currentCrt);
    if (err != aos::ErrorEnum::eNone) {
        return err;
    }

    err = GetX509CertExtensions(cert, currentCrt);
    if (err != aos::ErrorEnum::eNone) {
        return err;
    }

    return aos::ErrorEnum::eNone;
}

aos::Error CryptUtils::GetX509CertSubject(aos::crypto::x509::Certificate& cert, mbedtls_x509_crt* crt)
{
    cert.mSubject.Resize(crt->subject_raw.len);

    memcpy(cert.mSubject.Get(), crt->subject_raw.p, crt->subject_raw.len);

    return aos::ErrorEnum::eNone;
}

aos::Error CryptUtils::GetX509CertIssuer(aos::crypto::x509::Certificate& cert, mbedtls_x509_crt* crt)
{
    cert.mIssuer.Resize(crt->issuer_raw.len);

    memcpy(cert.mIssuer.Get(), crt->issuer_raw.p, crt->issuer_raw.len);

    return aos::ErrorEnum::eNone;
}

aos::Error CryptUtils::GetX509CertSerialNumber(aos::crypto::x509::Certificate& cert, mbedtls_x509_crt* crt)
{
    cert.mSerial.Resize(crt->serial.len);

    memcpy(cert.mSerial.Get(), crt->serial.p, crt->serial.len);

    return aos::ErrorEnum::eNone;
}

aos::Error CryptUtils::GetX509CertExtensions(aos::crypto::x509::Certificate& cert, mbedtls_x509_crt* crt)
{
    mbedtls_asn1_buf      buf = crt->v3_ext;
    mbedtls_asn1_sequence extns;

    auto ret = mbedtls_asn1_get_sequence_of(
        &buf.p, buf.p + buf.len, &extns, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE);
    if (ret != 0) {
        return AOS_ERROR_WRAP(ret);
    }

    mbedtls_asn1_sequence* next = &extns;

    while (next != nullptr) {
        size_t tagLen {};

        auto err = mbedtls_asn1_get_tag(&(next->buf.p), next->buf.p + next->buf.len, &tagLen, MBEDTLS_ASN1_OID);
        if (err != 0) {
            return AOS_ERROR_WRAP(err);
        }

        if (!memcmp(next->buf.p, MBEDTLS_OID_SUBJECT_KEY_IDENTIFIER, tagLen)) {
            unsigned char* p = next->buf.p + tagLen;
            err = mbedtls_asn1_get_tag(&p, p + next->buf.len - 2 - tagLen, &tagLen, MBEDTLS_ASN1_OCTET_STRING);
            if (err != 0) {
                return AOS_ERROR_WRAP(err);
            }

            err = mbedtls_asn1_get_tag(&p, p + next->buf.len - 2, &tagLen, MBEDTLS_ASN1_OCTET_STRING);
            if (err != 0) {
                return AOS_ERROR_WRAP(err);
            }

            cert.mSubjectKeyId.Resize(tagLen);
            memcpy(cert.mSubjectKeyId.Get(), p, tagLen);

            if (!cert.mAuthorityKeyId.IsEmpty()) {
                break;
            }
        }

        if (!memcmp(next->buf.p, MBEDTLS_OID_AUTHORITY_KEY_IDENTIFIER, tagLen)) {
            unsigned char* p = next->buf.p + tagLen;
            size_t         len;

            err = mbedtls_asn1_get_tag(&p, next->buf.p + next->buf.len, &len, MBEDTLS_ASN1_OCTET_STRING);
            if (err != 0) {
                return AOS_ERROR_WRAP(err);
            }

            if (*p != (MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE)) {
                return AOS_ERROR_WRAP(MBEDTLS_ERR_ASN1_UNEXPECTED_TAG);
            }

            err = mbedtls_asn1_get_tag(
                &p, next->buf.p + next->buf.len, &len, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE);
            if (err != 0) {
                return AOS_ERROR_WRAP(err);
            }

            if (*p != (MBEDTLS_ASN1_CONTEXT_SPECIFIC | 0)) {
                return AOS_ERROR_WRAP(MBEDTLS_ERR_ASN1_UNEXPECTED_TAG);
            }

            err = mbedtls_asn1_get_tag(&p, next->buf.p + next->buf.len, &len, MBEDTLS_ASN1_CONTEXT_SPECIFIC | 0);
            if (err != 0) {
                return AOS_ERROR_WRAP(err);
            }

            cert.mAuthorityKeyId.Resize(len);
            memcpy(cert.mAuthorityKeyId.Get(), p, len);

            if (!cert.mSubjectKeyId.IsEmpty()) {
                break;
            }
        }

        next = next->next;
    }

    return aos::ErrorEnum::eNone;
}

aos::Error CryptUtils::InitializeCertificate(mbedtls_x509write_cert& cert, mbedtls_pk_context& pk,
    mbedtls_ctr_drbg_context& ctr_drbg, mbedtls_entropy_context& entropy)
{
    mbedtls_x509write_crt_init(&cert);
    mbedtls_pk_init(&pk);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_entropy_init(&entropy);
    const char* pers = "cert_generation";

    mbedtls_x509write_crt_set_md_alg(&cert, MBEDTLS_MD_SHA256);

    return AOS_ERROR_WRAP(mbedtls_ctr_drbg_seed(
        &ctr_drbg, mbedtls_entropy_func, &entropy, reinterpret_cast<const unsigned char*>(pers), strlen(pers)));
}

aos::Error CryptUtils::ParsePrivateKey(
    mbedtls_pk_context& pk, const aos::crypto::PrivateKey& privKey, mbedtls_ctr_drbg_context& ctrDrbg)
{
    return AOS_ERROR_WRAP(mbedtls_pk_parse_key(
        &pk, privKey.GetPublic().Get(), privKey.GetPublic().Size(), nullptr, 0, mbedtls_ctr_drbg_random, &ctrDrbg));
}

aos::Error CryptUtils::SetCertificateProperties(mbedtls_x509write_cert& cert, mbedtls_pk_context& pk,
    mbedtls_ctr_drbg_context& ctrDrbg, const aos::crypto::x509::Certificate& templ,
    const aos::crypto::x509::Certificate& parent)
{
    mbedtls_x509write_crt_set_subject_key(&cert, &pk);
    mbedtls_x509write_crt_set_issuer_key(&cert, &pk);

    auto err = SetCertificateSerialNumber(cert, ctrDrbg, templ);
    if (err != aos::ErrorEnum::eNone) {
        return err;
    }

    aos::StaticString<aos::crypto::cCertSubjSize> subject;
    err = DNToString(templ.mSubject, subject);
    if (err != aos::ErrorEnum::eNone) {
        return err;
    }

    auto ret = mbedtls_x509write_crt_set_subject_name(&cert, subject.CStr());
    if (ret != 0) {
        return AOS_ERROR_WRAP(ret);
    }

    aos::StaticString<aos::crypto::cCertSubjSize> issuer;
    err = DNToString((!parent.mSubject.IsEmpty() ? parent.mSubject : templ.mIssuer), issuer);
    if (err != aos::ErrorEnum::eNone) {
        return err;
    }

    ret = mbedtls_x509write_crt_set_issuer_name(&cert, issuer.CStr());
    if (ret != 0) {
        return AOS_ERROR_WRAP(ret);
    }

    err = SetCertificateSubjectKeyIdentifier(cert, templ);
    if (err != aos::ErrorEnum::eNone) {
        return err;
    }

    return SetCertificateAuthorityKeyIdentifier(cert, templ, parent);
}

aos::Error CryptUtils::WriteCertificatePem(
    mbedtls_x509write_cert& cert, mbedtls_ctr_drbg_context& ctrDrbg, aos::Array<uint8_t>& pemCert)
{
    unsigned char buffer[aos::crypto::cPEMCertSize];
    auto          ret = mbedtls_x509write_crt_pem(&cert, buffer, sizeof(buffer), mbedtls_ctr_drbg_random, &ctrDrbg);
    if (ret != 0) {
        return AOS_ERROR_WRAP(ret);
    }

    pemCert.Resize(strlen(reinterpret_cast<const char*>(buffer)) + 1);
    memcpy(pemCert.Get(), buffer, pemCert.Size());

    return aos::ErrorEnum::eNone;
}

aos::Error CryptUtils::SetCertificateSerialNumber(
    mbedtls_x509write_cert& cert, mbedtls_ctr_drbg_context& ctrDrbg, const aos::crypto::x509::Certificate& templ)
{
    if (templ.mSerial.IsEmpty()) {
        mbedtls_mpi serial;
        mbedtls_mpi_init(&serial);

        auto ret
            = mbedtls_mpi_fill_random(&serial, MBEDTLS_X509_RFC5280_MAX_SERIAL_LEN, mbedtls_ctr_drbg_random, &ctrDrbg);
        if (ret != 0) {
            return AOS_ERROR_WRAP(ret);
        }

        ret = mbedtls_x509write_crt_set_serial(&cert, &serial);
        mbedtls_mpi_free(&serial);

        return AOS_ERROR_WRAP(ret);
    }

    return AOS_ERROR_WRAP(mbedtls_x509write_crt_set_serial_raw(
        &cert, const_cast<aos::crypto::x509::Certificate&>(templ).mSerial.Get(), templ.mSerial.Size()));
}

aos::Error CryptUtils::SetCertificateSubjectKeyIdentifier(
    mbedtls_x509write_cert& cert, const aos::crypto::x509::Certificate& templ)
{
    if (templ.mSubjectKeyId.IsEmpty()) {
        return AOS_ERROR_WRAP(mbedtls_x509write_crt_set_subject_key_identifier(&cert));
    }

    return AOS_ERROR_WRAP(mbedtls_x509write_crt_set_extension(&cert, MBEDTLS_OID_SUBJECT_KEY_IDENTIFIER,
        MBEDTLS_OID_SIZE(MBEDTLS_OID_SUBJECT_KEY_IDENTIFIER), 0, templ.mSubjectKeyId.Get(),
        templ.mSubjectKeyId.Size()));
}

aos::Error CryptUtils::SetCertificateAuthorityKeyIdentifier(mbedtls_x509write_cert& cert,
    const aos::crypto::x509::Certificate& templ, const aos::crypto::x509::Certificate& parent)
{
    if (!parent.mSubjectKeyId.IsEmpty()) {
        return AOS_ERROR_WRAP(mbedtls_x509write_crt_set_extension(&cert, MBEDTLS_OID_AUTHORITY_KEY_IDENTIFIER,
            MBEDTLS_OID_SIZE(MBEDTLS_OID_AUTHORITY_KEY_IDENTIFIER), 0, parent.mSubjectKeyId.Get(),
            parent.mSubjectKeyId.Size()));
    }

    if (templ.mAuthorityKeyId.IsEmpty()) {
        return AOS_ERROR_WRAP(mbedtls_x509write_crt_set_authority_key_identifier(&cert));
    }

    return AOS_ERROR_WRAP(mbedtls_x509write_crt_set_extension(&cert, MBEDTLS_OID_AUTHORITY_KEY_IDENTIFIER,
        MBEDTLS_OID_SIZE(MBEDTLS_OID_AUTHORITY_KEY_IDENTIFIER), 0, templ.mAuthorityKeyId.Get(),
        templ.mAuthorityKeyId.Size()));
}

aos::Error CryptUtils::InitializeCSR(mbedtls_x509write_csr& csr, mbedtls_pk_context& pk,
    mbedtls_ctr_drbg_context& ctrDrbg, mbedtls_entropy_context& entropy)
{
    mbedtls_x509write_csr_init(&csr);
    mbedtls_pk_init(&pk);
    mbedtls_ctr_drbg_init(&ctrDrbg);
    mbedtls_entropy_init(&entropy);

    mbedtls_x509write_csr_set_md_alg(&csr, MBEDTLS_MD_SHA256);

    const char* pers = "csr_generation";

    return AOS_ERROR_WRAP(mbedtls_ctr_drbg_seed(
        &ctrDrbg, mbedtls_entropy_func, &entropy, reinterpret_cast<const unsigned char*>(pers), strlen(pers)));
}

aos::Error CryptUtils::SetCSRProperties(
    mbedtls_x509write_csr& csr, mbedtls_pk_context& pk, const aos::crypto::x509::CSR& templ)
{
    mbedtls_x509write_csr_set_key(&csr, &pk);

    aos::StaticString<aos::crypto::cCertSubjSize> subject;
    auto                                          err = DNToString(templ.mSubject, subject);
    if (err != aos::ErrorEnum::eNone) {
        return err;
    }

    auto ret = mbedtls_x509write_csr_set_subject_name(&csr, subject.CStr());
    if (ret != 0) {
        return AOS_ERROR_WRAP(ret);
    }

    err = SetCSRAlternativeNames(csr, templ);
    if (err != aos::ErrorEnum::eNone) {
        return err;
    }

    return SetCSRExtraExtensions(csr, templ);
}

aos::Error CryptUtils::SetCSRAlternativeNames(mbedtls_x509write_csr& csr, const aos::crypto::x509::CSR& templ)
{
    mbedtls_x509_san_list   sanList[aos::crypto::cAltDNSNamesCount];
    aos::crypto::x509::CSR& tmpl = const_cast<aos::crypto::x509::CSR&>(templ);
    size_t                  dnsNameCount = tmpl.mDNSNames.Size();

    for (size_t i = 0; i < tmpl.mDNSNames.Size(); i++) {
        sanList[i].node.type = MBEDTLS_X509_SAN_DNS_NAME;
        sanList[i].node.san.unstructured_name.tag = MBEDTLS_ASN1_IA5_STRING;
        sanList[i].node.san.unstructured_name.len = tmpl.mDNSNames[i].Size();
        sanList[i].node.san.unstructured_name.p = reinterpret_cast<unsigned char*>(tmpl.mDNSNames[i].Get());

        sanList[i].next = (i < dnsNameCount - 1) ? &sanList[i + 1] : nullptr;
    }

    return AOS_ERROR_WRAP(mbedtls_x509write_csr_set_subject_alternative_name(&csr, sanList));
}

aos::Error CryptUtils::SetCSRExtraExtensions(mbedtls_x509write_csr& csr, const aos::crypto::x509::CSR& templ)
{
    for (const auto& extension : templ.mExtraExtensions) {
        const char*          oid = extension.mId.CStr();
        const unsigned char* value = extension.mValue.Get();
        size_t               oidLen = extension.mId.Size();
        size_t               valueLen = extension.mValue.Size();

        int ret = mbedtls_x509write_csr_set_extension(&csr, oid, oidLen, 0, value, valueLen);
        if (ret != 0) {
            return AOS_ERROR_WRAP(ret);
        }
    }

    return aos::ErrorEnum::eNone;
}

aos::Error CryptUtils::WriteCSRPem(
    mbedtls_x509write_csr& csr, mbedtls_ctr_drbg_context& ctrDrbg, aos::Array<uint8_t>& pemCSR)
{
    unsigned char buffer[aos::crypto::cPemCSRSize];
    auto          ret = mbedtls_x509write_csr_pem(&csr, buffer, sizeof(buffer), mbedtls_ctr_drbg_random, &ctrDrbg);
    if (ret != 0) {
        return AOS_ERROR_WRAP(ret);
    }

    pemCSR.Resize(strlen(reinterpret_cast<const char*>(buffer)) + 1);
    memcpy(pemCSR.Get(), buffer, pemCSR.Size());

    return aos::ErrorEnum::eNone;
}
