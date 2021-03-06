//
// VMime library (http://www.vmime.org)
// Copyright (C) 2002-2013 Vincent Richard <vincent@vmime.org>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 3 of
// the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// Linking this library statically or dynamically with other modules is making
// a combined work based on this library.  Thus, the terms and conditions of
// the GNU General Public License cover the whole combination.
//

#ifndef VMIME_NET_TLS_TLSSOCKET_OPENSSL_HPP_INCLUDED
#define VMIME_NET_TLS_TLSSOCKET_OPENSSL_HPP_INCLUDED


#ifndef VMIME_BUILDING_DOC


#include "vmime/config.hpp"


#if VMIME_HAVE_MESSAGING_FEATURES && VMIME_HAVE_TLS_SUPPORT && VMIME_TLS_SUPPORT_LIB_IS_OPENSSL


#include "vmime/net/tls/TLSSocket.hpp"

#include <memory>

#include <openssl/ssl.h>


namespace vmime {
namespace net {
namespace tls {


class TLSSession;
class TLSSession_OpenSSL;


class TLSSocket_OpenSSL : public TLSSocket
{
public:

	TLSSocket_OpenSSL(shared_ptr <TLSSession_OpenSSL> session, shared_ptr <socket> sok);
	~TLSSocket_OpenSSL();


	void handshake(shared_ptr <timeoutHandler> toHandler = null);

	shared_ptr <security::cert::certificateChain> getPeerCertificates() const;

	// Implementation of 'socket'
	void connect(const string& address, const port_t port);
	void disconnect();
	bool isConnected() const;

	void receive(string& buffer);
	size_type receiveRaw(char* buffer, const size_type count);

	void send(const string& buffer);
	void sendRaw(const char* buffer, const size_type count);
	size_type sendRawNonBlocking(const char* buffer, const size_type count);

	size_type getBlockSize() const;

	unsigned int getStatus() const;

	const string getPeerName() const;
	const string getPeerAddress() const;

private:

	static BIO_METHOD sm_customBIOMethod;

	static int bio_write(BIO* bio, const char* buf, int len);
	static int bio_read(BIO* bio, char* buf, int len);
	static int bio_puts(BIO* bio, const char* str);
	static int bio_gets(BIO* bio, char* buf, int len);
	static long bio_ctrl(BIO* bio, int cmd, long num, void* ptr);
	static int bio_create(BIO* bio);
	static int bio_destroy(BIO* bio);

	void createSSLHandle();

	void internalThrow();
	void handleError(int rc);


	shared_ptr <TLSSession_OpenSSL> m_session;

	shared_ptr <socket> m_wrapped;

	bool m_connected;

	char m_buffer[65536];

	shared_ptr <timeoutHandler> m_toHandler;

	SSL* m_ssl;

	// Last exception thrown from C BIO functions
	std::auto_ptr <std::exception> m_ex;
};


} // tls
} // net
} // vmime


#endif // VMIME_HAVE_MESSAGING_FEATURES && VMIME_HAVE_TLS_SUPPORT && VMIME_TLS_SUPPORT_LIB_IS_OPENSSL

#endif // VMIME_BUILDING_DOC

#endif // VMIME_NET_TLS_TLSSOCKET_OPENSSL_HPP_INCLUDED

