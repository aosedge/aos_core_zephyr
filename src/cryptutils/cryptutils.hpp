/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CRYPTUTILS_HPP_
#define CRYPTUTILS_HPP_

#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/x509_csr.h>

#include "aos/common/crypto.hpp"

class CryptUtils : public aos::crypto::x509::ProviderItf {
public:
    /**
     * Creates a new certificate based on a template.
     *
     * @param templ a pattern for a new certificate.
     * @param parent a parent certificate in a certificate chain.
     * @param pubKey public key.
     * @param privKey private key.
     * @param[out] resultCert result certificate in PEM format.
     * @result Error.
     */
    aos::Error CreateCertificate(const aos::crypto::x509::Certificate& templ,
        const aos::crypto::x509::Certificate& parent, const aos::crypto::PrivateKey& privKey,
        aos::Array<uint8_t>& pemCert) override;

    /**
     * Reads certificates from a PEM blob.
     *
     * @param pemBlob raw certificates in a PEM format.
     * @param[out] resultCerts result certificate chain.
     * @result Error.
     */
    aos::Error PEMToX509Certs(
        const aos::Array<uint8_t>& pemBlob, aos::Array<aos::crypto::x509::Certificate>& resultCerts) override;

    /**
     * Creates a new certificate request, based on a template.
     *
     * @param templ template for a new certificate request.
     * @param privKey private key.
     * @param[out] pemCSR result CSR in PEM format.
     * @result Error.
     */
    aos::Error CreateCSR(const aos::crypto::x509::CSR& templ, const aos::crypto::PrivateKey& privKey,
        aos::Array<uint8_t>& pemCSR) override;

    /**
     * Constructs x509 distinguished name(DN) from the argument list.
     *
     * @param comName common name.
     * @param[out] result result DN.
     * @result Error.
     */
    aos::Error CreateDN(const aos::String& commonName, aos::Array<uint8_t>& result) override;

    /**
     * Returns text representation of x509 distinguished name(DN).
     *
     * @param dn source binary representation of DN.
     * @param[out] result DN text representation.
     * @result Error.
     */
    aos::Error DNToString(const aos::Array<uint8_t>& dn, aos::String& result) override;

private:
    aos::Error GetOIDString(aos::Array<uint8_t>& oid, aos::String& result);
    aos::Error ParseX509Certs(mbedtls_x509_crt* currentCrt, aos::crypto::x509::Certificate& cert);
    aos::Error GetX509CertSubject(aos::crypto::x509::Certificate& cert, mbedtls_x509_crt* crt);
    aos::Error GetX509CertIssuer(aos::crypto::x509::Certificate& cert, mbedtls_x509_crt* crt);
    aos::Error GetX509CertSerialNumber(aos::crypto::x509::Certificate& cert, mbedtls_x509_crt* crt);
    aos::Error GetX509CertExtensions(aos::crypto::x509::Certificate& cert, mbedtls_x509_crt* crt);

    aos::Error ParsePrivateKey(
        mbedtls_pk_context& pk, const aos::crypto::PrivateKey& privKey, mbedtls_ctr_drbg_context& ctrDrbg);

    aos::Error InitializeCertificate(mbedtls_x509write_cert& cert, mbedtls_pk_context& pk,
        mbedtls_ctr_drbg_context& ctr_drbg, mbedtls_entropy_context& entropy);
    aos::Error SetCertificateProperties(mbedtls_x509write_cert& cert, mbedtls_pk_context& pk,
        mbedtls_ctr_drbg_context& ctrDrbg, const aos::crypto::x509::Certificate& templ,
        const aos::crypto::x509::Certificate& parent);
    aos::Error WriteCertificatePem(
        mbedtls_x509write_cert& cert, mbedtls_ctr_drbg_context& ctrDrbg, aos::Array<uint8_t>& pemCert);
    aos::Error SetCertificateSerialNumber(
        mbedtls_x509write_cert& cert, mbedtls_ctr_drbg_context& ctrDrbg, const aos::crypto::x509::Certificate& templ);
    aos::Error SetCertificateSubjectKeyIdentifier(
        mbedtls_x509write_cert& cert, const aos::crypto::x509::Certificate& templ);
    aos::Error SetCertificateAuthorityKeyIdentifier(mbedtls_x509write_cert& cert,
        const aos::crypto::x509::Certificate& templ, const aos::crypto::x509::Certificate& parent);

    aos::Error InitializeCSR(mbedtls_x509write_csr& csr, mbedtls_pk_context& pk, mbedtls_ctr_drbg_context& ctrDrbg,
        mbedtls_entropy_context& entropy);
    aos::Error SetCSRProperties(
        mbedtls_x509write_csr& csr, mbedtls_pk_context& pk, const aos::crypto::x509::CSR& templ);
    aos::Error SetCSRAlternativeNames(mbedtls_x509write_csr& csr, const aos::crypto::x509::CSR& templ);
    aos::Error SetCSRExtraExtensions(mbedtls_x509write_csr& csr, const aos::crypto::x509::CSR& templ);
    aos::Error WriteCSRPem(mbedtls_x509write_csr& csr, mbedtls_ctr_drbg_context& ctrDrbg, aos::Array<uint8_t>& pemCSR);
};

#endif
