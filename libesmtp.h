#ifndef _libesmtp_h
#define _libesmtp_h
/*
 *  This file is part of libESMTP, a library for submission of RFC 2822
 *  formatted electronic mail messages using the SMTP protocol described
 *  in RFC 2821.
 *
 *  Copyright (C) 2001,2002  Brian Stafford  <brian@stafford.uklinux.net>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*  *********************************************************************
 *  ***************************** IMPORTANT *****************************
 *  *********************************************************************
 *
 *  This API is intended for use as an ESMTP client within a Mail User
 *  Agent (MUA) or other program that wishes to submit mail to a
 *  preconfigured Mail Transport Agent (MTA).
 *
 *  ***** IT IS NOT INTENDED FOR USE IN CODE THAT IMPLEMENTS AN MTA *****
 *
 *  In particular, the CNAME, MX and A/AAAA lookup rules an MTA must
 *  use to locate the next hop MTA are not implemented.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * DOC: libESMTP Types
 *
 * libESMTP Types
 * ==============
 *
 * The following types are defined by libESMTP.
 */

typedef struct smtp_session *smtp_session_t;
typedef struct smtp_message *smtp_message_t;
typedef struct smtp_recipient *smtp_recipient_t;

/**
 * typedef smtp_enumerate_messagecb_t - Message callback.
 * @message: The session.
 * @arg: User data passed to smtp_enumerate_messages().
 *
 * Callback function to handle a message.
 */
typedef void (*smtp_enumerate_messagecb_t) (smtp_message_t message, void *arg);

/**
 * typedef smtp_enumerate_recipientcb_t - Recipient callback.
 * @recipient: The recipient.
 * @mailbox: Mailbox name.
 * @arg: User data passed to smtp_enumerate_recipients().
 *
 * Callback to process a recipient.
 */
typedef void (*smtp_enumerate_recipientcb_t) (smtp_recipient_t recipient,
					      const char *mailbox, void *arg);

enum { Ver_VERSION, Ver_SO_VERSION, Ver_LT_VERSION };
int smtp_version (void *buf, size_t len, int what);

smtp_session_t smtp_create_session (void);
smtp_message_t smtp_add_message (smtp_session_t session);
int smtp_enumerate_messages (smtp_session_t session,
			     smtp_enumerate_messagecb_t cb, void *arg);
int smtp_set_server (smtp_session_t session, const char *hostport);
const char *smtp_get_server_name (smtp_session_t session);
int smtp_set_hostname (smtp_session_t session, const char *hostname);
int smtp_set_reverse_path (smtp_message_t message, const char *mailbox);
smtp_recipient_t smtp_add_recipient (smtp_message_t message,
                                     const char *mailbox);
int smtp_enumerate_recipients (smtp_message_t message,
			       smtp_enumerate_recipientcb_t cb, void *arg);
int smtp_set_header (smtp_message_t message, const char *header, ...);
enum header_option
  {
    Hdr_OVERRIDE,
    Hdr_PROHIBIT
    /* add new options here */
  };
int smtp_set_header_option (smtp_message_t message, const char *header,
			    enum header_option option, ...);
int smtp_set_resent_headers (smtp_message_t message, int onoff);
typedef const char *(*smtp_messagecb_t) (void **ctx, int *len, void *arg);
int smtp_set_messagecb (smtp_message_t message,
		        smtp_messagecb_t cb, void *arg);
enum
  {
  /* Protocol progress */
    SMTP_EV_CONNECT,
    SMTP_EV_MAILSTATUS,
    SMTP_EV_RCPTSTATUS,
    SMTP_EV_MESSAGEDATA,
    SMTP_EV_MESSAGESENT,
    SMTP_EV_DISCONNECT,
    SMTP_EV_SYNTAXWARNING,

  /* Protocol extension progress */
    SMTP_EV_ETRNSTATUS = 1000,

  /* Required extensions */
    SMTP_EV_EXTNA_DSN = 2000,
    SMTP_EV_EXTNA_8BITMIME,
    SMTP_EV_EXTNA_STARTTLS,
    SMTP_EV_EXTNA_ETRN,
    SMTP_EV_EXTNA_CHUNKING,
    SMTP_EV_EXTNA_BINARYMIME,

  /* Extensions specific events */
    SMTP_EV_DELIVERBY_EXPIRED = 3000,

  /* STARTTLS */
    SMTP_EV_WEAK_CIPHER = 3100,
    SMTP_EV_STARTTLS_OK,
    SMTP_EV_INVALID_PEER_CERTIFICATE,
    SMTP_EV_NO_PEER_CERTIFICATE,
    SMTP_EV_WRONG_PEER_CERTIFICATE,
    SMTP_EV_NO_CLIENT_CERTIFICATE,
    SMTP_EV_UNUSABLE_CLIENT_CERTIFICATE,
    SMTP_EV_UNUSABLE_CA_LIST
  };
typedef void (*smtp_eventcb_t) (smtp_session_t session, int event_no,
				void *arg, ...);
int smtp_set_eventcb (smtp_session_t session, smtp_eventcb_t cb, void *arg);
typedef void (*smtp_monitorcb_t) (const char *buf, int buflen,
				  int writing, void *arg);
int smtp_set_monitorcb (smtp_session_t session, smtp_monitorcb_t cb, void *arg,
			int headers);
int smtp_start_session (smtp_session_t session);
int smtp_destroy_session (smtp_session_t session);

struct smtp_status
  {
    int code;			/* SMTP protocol status code */
    char *text;			/* Text from the server */
    int enh_class;		/* RFC 2034 enhanced status code triplet */
    int enh_subject;
    int enh_detail;
  };
typedef struct smtp_status smtp_status_t;

const smtp_status_t *smtp_message_transfer_status (smtp_message_t message);
const smtp_status_t *smtp_reverse_path_status (smtp_message_t message);
int smtp_message_reset_status (smtp_message_t recipient);
const smtp_status_t *smtp_recipient_status (smtp_recipient_t recipient);
int smtp_recipient_check_complete (smtp_recipient_t recipient);
int smtp_recipient_reset_status (smtp_recipient_t recipient);
int smtp_errno (void);
char *smtp_strerror (int error, char buf[], size_t buflen);


#ifdef LIBESMTP_ENABLE_DEPRECATED_SYMBOLS
void *smtp_set_application_data (smtp_session_t session, void *data);
#endif
void smtp_set_application_data_release (smtp_session_t session, void *data,
                                        void (*release) (void *));
void *smtp_get_application_data (smtp_session_t session);
#ifdef LIBESMTP_ENABLE_DEPRECATED_SYMBOLS
void *smtp_message_set_application_data (smtp_message_t message, void *data);
#endif
void smtp_message_set_application_data_release (smtp_message_t message,
                                                void *data,
                                                void (*release) (void *));
void *smtp_message_get_application_data (smtp_message_t message);
#ifdef LIBESMTP_ENABLE_DEPRECATED_SYMBOLS
void *smtp_recipient_set_application_data (smtp_recipient_t recipient,
					   void *data);
#endif
void smtp_recipient_set_application_data_release (smtp_recipient_t recipient,
                                                  void *data,
                                                  void (*release) (void *));
void *smtp_recipient_get_application_data (smtp_recipient_t recipient);

int smtp_option_require_all_recipients (smtp_session_t session, int state);

#ifdef _auth_client_h
int smtp_auth_set_context (smtp_session_t session, auth_context_t context);
#endif

/*
   Standard callbacks for reading the message from the application.
 */

const char *_smtp_message_fp_cb (void **ctx, int *len, void *arg);
#define smtp_set_message_fp(message,fp)	\
		smtp_set_messagecb ((message), _smtp_message_fp_cb, (fp))

const char *_smtp_message_str_cb (void **ctx, int *len, void *arg);
#define smtp_set_message_str(message,str)	\
		smtp_set_messagecb ((message), _smtp_message_str_cb, (str))

/* Protocol timeouts */

/**
 * enum rfc2822_timeouts - Specify timeout.
 * @Timeout_GREETING: Timeout waiting for server greeting.
 * @Timeout_ENVELOPE: Timeout for envelope responses.
 * @Timeout_DATA: Timeout waiting for data transfer to begin.
 * @Timeout_TRANSFER: Timeout for data transfer phase.
 * @Timeout_DATA2: Timeout for data transfer phase.
 *
 * Timeout flags. In addition %Timeout_OVERRIDE_RFC2822_MINIMUM may
 * be bitwise-ORed with above to override recommended minimum timeouts.
 */
enum rfc2822_timeouts
  {
    Timeout_GREETING,
    Timeout_ENVELOPE,
    Timeout_DATA,
    Timeout_TRANSFER,
    Timeout_DATA2
  };
#define Timeout_OVERRIDE_RFC2822_MINIMUM	0x1000
long smtp_set_timeout (smtp_session_t session, int which, long value);

/****************************************************************************
 * The following APIs relate to SMTP extensions.  Note that not all
 * supported extensions have corresponding API functions.
 ****************************************************************************/

/*
    	RFC 3461.  Delivery Status Notification (DSN)
 */

/**
 * enum ret_flags - DSN request flags.
 * @Ret_NOTSET: DSN is not requested.
 * @Ret_FULL: Request full message in DSN.
 * @Ret_HDRS: Request only headers in DSN.
 *
 * DSN flags specifying requested notification.
 */
enum ret_flags { Ret_NOTSET, Ret_FULL, Ret_HDRS };
int smtp_dsn_set_ret (smtp_message_t message, enum ret_flags flags);
int smtp_dsn_set_envid (smtp_message_t message, const char *envid);

/**
 * enum notify_flags - DSN notification conditions.
 * @Notify_NOTSET: Notify options not requested.
 * @Notify_NEVER: Never notify.
 * @Notify_SUCCESS: Notify delivery success.
 * @Notify_FAILURE: Notify delivery failure.
 * @Notify_DELAY: Notify delivery is delayed.
 *
 * DSN notify flags.
 */
enum notify_flags
  {
    Notify_NOTSET,
    Notify_NEVER	= -1,
    Notify_SUCCESS	= 1,
    Notify_FAILURE	= 2,
    Notify_DELAY	= 4
  };
int smtp_dsn_set_notify (smtp_recipient_t recipient, enum notify_flags flags);
int smtp_dsn_set_orcpt (smtp_recipient_t recipient,
			const char *address_type, const char *address);

/*
    	RFC 1870.  SMTP Size extension.
 */

int smtp_size_set_estimate (smtp_message_t message, unsigned long size);

/*
	RFC 6152.  8bit-MIME Transport
 */

/**
 * enum e8bitmime_body - Message body type.
 * @E8bitmime_NOTSET: Body type not declared.
 * @E8bitmime_7BIT: Body conforms with RFC 5322.
 * @E8bitmime_8BITMIME: Body uses 8 bit encoding.
 * @E8bitmime_BINARYMIME: Body uses BINARYMIME encoding.
 *
 * 8BITMIME extension flags.
 */
enum e8bitmime_body
  {
    E8bitmime_NOTSET,
    E8bitmime_7BIT,
    E8bitmime_8BITMIME,
    E8bitmime_BINARYMIME
  };
int smtp_8bitmime_set_body (smtp_message_t message, enum e8bitmime_body body);

/*
	RFC 2852.  Deliver By
 */

/**
 * enum by_mode - Delivery flags.
 * @By_NOTSET: Deliver-by notification not requested.
 * @By_NOTIFY: FIXME
 * @By_RETURN: FIXME
 *
 * DELIVERBY (RFC 2852) extension flags.
 */
enum by_mode
  {
    By_NOTSET,
    By_NOTIFY,
    By_RETURN
  };
int smtp_deliverby_set_mode (smtp_message_t message,
			     long time, enum by_mode mode, int trace);

/*
    	RFC 3207.  SMTP Starttls extension.
 */
/**
 * enum starttls_option - TLS mode options
 * @Starttls_DISABLED: Do not use TLS, even if offered by the MTA.
 * @Starttls_ENABLED: Use TLS if offered by the MTA.
 * @Starttls_REQUIRED: Exit session if TLS is not offered by the MTA.
 */
enum starttls_option
  {
    Starttls_DISABLED,
    Starttls_ENABLED,
    Starttls_REQUIRED
  };
int smtp_starttls_enable (smtp_session_t session, enum starttls_option how);

/* Only delare this if the app has incuded <openssl/ssl.h> which
   defines the symbol tested. */
#ifdef SSL_SESSION_ASN1_VERSION
int smtp_starttls_set_ctx (smtp_session_t session, SSL_CTX *ctx);
#endif

/**
 * typedef smtp_starttls_passwordcb_t - Password callback signature.
 * @buf: Buffer to receive the password.
 * @buflen: Length of the buffer.
 * @rwflag: 0 for reading/decryption, 1 for writing/encryption.
 * @arg: User data passed to smtp_starttls_set_password_cb().
 *
 * The callback function declaration is cleverly chosen to be the same as
 * the OpenSSL declaration.
 *
 * Return: Actual length of password.
 */
typedef int (*smtp_starttls_passwordcb_t) (char *buf, int buflen,
                                           int rwflag, void *arg);
int smtp_starttls_set_password_cb (smtp_starttls_passwordcb_t cb, void *arg);

/*
    	RFC 1985.  Remote Message Queue Starting (ETRN)
 */

typedef struct smtp_etrn_node *smtp_etrn_node_t;
smtp_etrn_node_t smtp_etrn_add_node (smtp_session_t session,
                                     int option, const char *node);
typedef void (*smtp_etrn_enumerate_nodecb_t) (smtp_etrn_node_t node,
					      int option, const char *domain,
					      void *arg);
int smtp_etrn_enumerate_nodes (smtp_session_t session,
			       smtp_etrn_enumerate_nodecb_t cb, void *arg);
const smtp_status_t *smtp_etrn_node_status (smtp_etrn_node_t node);

#ifdef LIBESMTP_ENABLE_DEPRECATED_SYMBOLS
void *smtp_etrn_set_application_data (smtp_etrn_node_t node, void *data);
#endif
void smtp_etrn_set_application_data_release (smtp_etrn_node_t node, void *data,
                                             void (*release) (void *));
void *smtp_etrn_get_application_data (smtp_etrn_node_t node);

#ifdef __cplusplus
};
#endif

#define SMTPAPI_CHECK_ARGS(test,ret)			\
	do 						\
		if (!(test)) {				\
			set_error (SMTP_ERR_INVAL);	\
			return ret;			\
		}					\
	while (0)


#define SMTP_ERR_NOTHING_TO_DO			2
#define SMTP_ERR_DROPPED_CONNECTION		3
#define SMTP_ERR_INVALID_RESPONSE_SYNTAX	4
#define SMTP_ERR_STATUS_MISMATCH		5
#define SMTP_ERR_INVALID_RESPONSE_STATUS	6
#define SMTP_ERR_INVAL				7
#define SMTP_ERR_EXTENSION_NOT_AVAILABLE	8

/* Deprecated - these will be removed in a future release */
#define SMTP_ERR_HOST_NOT_FOUND			SMTP_ERR_EAI_ADDRFAMILY
#define SMTP_ERR_NO_ADDRESS			SMTP_ERR_EAI_NODATA
#define SMTP_ERR_NO_RECOVERY			SMTP_ERR_EAI_FAIL
#define SMTP_ERR_TRY_AGAIN			SMTP_ERR_EAI_AGAIN

/* libESMTP versions of some getaddrinfo error numbers */
#define SMTP_ERR_EAI_AGAIN			12
#define SMTP_ERR_EAI_FAIL			11
#define SMTP_ERR_EAI_MEMORY			13
#define SMTP_ERR_EAI_ADDRFAMILY			9
#define SMTP_ERR_EAI_NODATA			10
#define SMTP_ERR_EAI_FAMILY			14
#define SMTP_ERR_EAI_BADFLAGS			15
#define SMTP_ERR_EAI_NONAME			16
#define SMTP_ERR_EAI_SERVICE			17
#define SMTP_ERR_EAI_SOCKTYPE			18

#define SMTP_ERR_UNTERMINATED_RESPONSE		19
#define SMTP_ERR_CLIENT_ERROR			20

/* Protocol monitor callback.  Values for writing */
#define SMTP_CB_READING				0
#define SMTP_CB_WRITING				1
#define SMTP_CB_HEADERS				2

#endif
