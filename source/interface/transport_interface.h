/*
 * coreMQTT <DEVELOPMENT BRANCH>
 * Copyright (C) 2022 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * @file transport_interface.h
 * @brief Transport interface definitions to send and receive data over the
 * network.
 */
#ifndef TRANSPORT_INTERFACE_H_
#define TRANSPORT_INTERFACE_H_

#include <stdint.h>
#include <stddef.h>

/* *INDENT-OFF* */
#ifdef __cplusplus
    extern "C" {
#endif
/* *INDENT-ON* */

/**
 * @transportpage
 * @brief The transport interface definition.
 *
 * @transportsectionoverview
 *
 * The transport interface is a set of APIs that must be implemented using an
 * external transport layer protocol. The transport interface is defined in
 * @ref transport_interface.h. This interface allows protocols like MQTT and
 * HTTP to send and receive data over the transport layer. This
 * interface does not handle connection and disconnection to the server of
 * interest. The connection, disconnection, and other transport settings, like
 * timeout and TLS setup, must be handled in the user application.
 * <br>
 *
 * The functions that must be implemented are:<br>
 * - [Transport Receive](@ref TransportRecv_t)
 * - [Transport Send](@ref TransportSend_t)
 *
 * Each of the functions above take in an opaque context @ref NetworkContext_t.
 * The functions above and the context are also grouped together in the
 * @ref TransportInterface_t structure:<br><br>
 * @snippet this define_transportinterface
 * <br>
 *
 * @transportsectionimplementation
 *
 * The following steps give guidance on implementing the transport interface:
 *
 * -# Implementing @ref NetworkContext_t<br><br>
 * @snippet this define_networkcontext
 * <br>
 * @ref NetworkContext_t is the incomplete type <b>struct NetworkContext</b>.
 * The implemented struct NetworkContext must contain all of the information
 * that is needed to receive and send data with the @ref TransportRecv_t
 * and the @ref TransportSend_t implementations.<br>
 * In the case of TLS over TCP, struct NetworkContext is typically implemented
 * with the TCP socket context and a TLS context.<br><br>
 * <b>Example code:</b>
 * @code{c}
 * struct NetworkContext
 * {
 *     struct MyTCPSocketContext tcpSocketContext;
 *     struct MyTLSContext tlsContext;
 * };
 * @endcode
 * <br>
 * -# Implementing @ref TransportRecv_t<br><br>
 * @snippet this define_transportrecv
 * <br>
 * This function is expected to populate a buffer, with bytes received from the
 * transport, and return the number of bytes placed in the buffer.
 * In the case of TLS over TCP, @ref TransportRecv_t is typically implemented by
 * calling the TLS layer function to receive data. In case of plaintext TCP
 * without TLS, it is typically implemented by calling the TCP layer receive
 * function. @ref TransportRecv_t may be invoked multiple times by the protocol
 * library, if fewer bytes than were requested to receive are returned.
 * Please note that it is HIGHLY RECOMMENDED that the transport receive implementation does NOT block.
 * <br><br>
 * <b>Example code:</b>
 * @code{c}
 * int32_t myNetworkRecvImplementation( NetworkContext_t * pNetworkContext,
 *                                      void * pBuffer,
 *                                      size_t bytesToRecv )
 * {
 *     int32_t bytesReceived = 0;
 *     bool callTlsRecvFunc = true;
 *
 *     // For a single byte read request, check if data is available on the network.
 *     if( bytesToRecv == 1 )
 *     {
 *        // If no data is available on the network, do not call TLSRecv
 *        // to avoid blocking for socket timeout.
 *        if( TLSRecvCount( pNetworkContext->tlsContext ) == 0 )
 *        {
 *            callTlsRecvFunc = false;
 *        }
 *     }
 *
 *     if( callTlsRecvFunc == true )
 *     {
 *        bytesReceived = TLSRecv( pNetworkContext->tlsContext,
 *                                 pBuffer,
 *                                 bytesToRecv,
 *                                 MY_SOCKET_TIMEOUT );
 *        if( bytesReceived < 0 )
 *        {
 *           // If the error code represents a timeout, then the return
 *           // code should be translated to zero so that the caller
 *           // can retry the read operation.
 *           if( bytesReceived == MY_SOCKET_ERROR_TIMEOUT )
 *           {
 *              bytesReceived = 0;
 *           }
 *        }
 *        // Handle other cases.
 *     }
 *     return bytesReceived;
 * }
 * @endcode
 * <br>
 * -# Implementing @ref TransportSend_t<br><br>
 * @snippet this define_transportsend
 * <br>
 * This function is expected to send the bytes, in the given buffer over the
 * transport, and return the number of bytes sent.
 * In the case of TLS over TCP, @ref TransportSend_t is typically implemented by
 * calling the TLS layer function to send data. In case of plaintext TCP
 * without TLS, it is typically implemented by calling the TCP layer send
 * function. @ref TransportSend_t may be invoked multiple times by the protocol
 * library, if fewer bytes than were requested to send are returned.
 * <br><br>
 * <b>Example code:</b>
 * @code{c}
 * int32_t myNetworkSendImplementation( NetworkContext_t * pNetworkContext,
 *                                      const void * pBuffer,
 *                                      size_t bytesToSend )
 * {
 *     int32_t bytesSent = 0;
 *     bytesSent = TLSSend( pNetworkContext->tlsContext,
 *                          pBuffer,
 *                          bytesToSend,
 *                          MY_SOCKET_TIMEOUT );
 *
 *      // If underlying TCP buffer is full, set the return value to zero
 *      // so that caller can retry the send operation.
 *     if( bytesSent == MY_SOCKET_ERROR_BUFFER_FULL )
 *     {
 *          bytesSent = 0;
 *     }
 *     else if( bytesSent < 0 )
 *     {
 *         // Handle socket error.
 *     }
 *     // Handle other cases.
 *
 *     return bytesSent;
 * }
 * @endcode
 */

/**
 * @transportstruct
 * @typedef NetworkContext_t
 * @brief The NetworkContext is an incomplete type. An implementation of this
 * interface must define struct NetworkContext for the system requirements.
 * This context is passed into the network interface functions.
 */
/* @[define_networkcontext] */
struct NetworkContext;
typedef struct NetworkContext NetworkContext_t;
/* @[define_networkcontext] */

/**
 * @transportcallback
 * @brief Transport interface for receiving data on the network.
 *
 * @note It is HIGHLY RECOMMENDED that the transport receive
 * implementation does NOT block.
 * coreMQTT will continue to call the transport interface if it receives
 * a partial packet until it accumulates enough data to get the complete
 * MQTT packet.
 * A non‐blocking implementation is also essential so that the library's inbuilt
 * keep‐alive mechanism can work properly, given the user chooses to use
 * that over their own keep alive mechanism.
 *
 * @param[in] pNetworkContext Implementation-defined network context.
 * @param[in] pBuffer Buffer to receive the data into.
 * @param[in] bytesToRecv Number of bytes requested from the network.
 *
 * @return The number of bytes received or a negative value to indicate
 * error.
 *
 * @note If no data is available on the network to read and no error
 * has occurred, zero MUST be the return value. A zero return value
 * SHOULD represent that the read operation can be retried by calling
 * the API function. Zero MUST NOT be returned if a network disconnection
 * has occurred.
 */
/* @[define_transportrecv] */
typedef int32_t ( * TransportRecv_t )( NetworkContext_t * pNetworkContext,
                                       void * pBuffer,
                                       size_t bytesToRecv );
/* @[define_transportrecv] */

/**
 * @transportcallback
 * @brief Transport interface for sending data over the network.
 *
 * @param[in] pNetworkContext Implementation-defined network context.
 * @param[in] pBuffer Buffer containing the bytes to send over the network stack.
 * @param[in] bytesToSend Number of bytes to send over the network.
 *
 * @return The number of bytes sent or a negative value to indicate error.
 *
 * @note If no data is transmitted over the network due to a full TX buffer and
 * no network error has occurred, this MUST return zero as the return value.
 * A zero return value SHOULD represent that the send operation can be retried
 * by calling the API function. Zero MUST NOT be returned if a network disconnection
 * has occurred.
 */
/* @[define_transportsend] */
typedef int32_t ( * TransportSend_t )( NetworkContext_t * pNetworkContext,
                                       const void * pBuffer,
                                       size_t bytesToSend );
/* @[define_transportsend] */

/**
 * @brief Transport vector structure for sending multiple messages.
 */
typedef struct TransportOutVector
{
    /**
     * @brief Base address of data.
     */
    const void * iov_base;

    /**
     * @brief Length of data in buffer.
     */
    size_t iov_len;
} TransportOutVector_t;

/**
 * @transportcallback
 * @brief Transport interface function for "vectored" / scatter-gather based
 * writes. This function is expected to iterate over the list of vectors pIoVec
 * having ioVecCount entries containing portions of one MQTT message at a maximum.
 * If the proper functionality is available, then the data in the list should be
 * copied to the underlying TCP buffer before flushing the buffer. Implementing it
 * in this fashion  will lead to sending of fewer TCP packets for all the values
 * in the list.
 *
 * @note If the proper write functionality is not present for a given device/IP-stack,
 * then there is no strict requirement to implement write. Only the send and recv
 * interfaces must be defined for the application to work properly.
 *
 * @param[in] pNetworkContext Implementation-defined network context.
 * @param[in] pIoVec An array of TransportIoVector_t structs.
 * @param[in] ioVecCount Number of TransportIoVector_t in pIoVec.
 *
 * @return The number of bytes written or a negative value to indicate error.
 *
 * @note If no data is written to the buffer due to the buffer being full this MUST
 * return zero as the return value.
 * A zero return value SHOULD represent that the write operation can be retried
 * by calling the API function. Zero MUST NOT be returned if a network disconnection
 * has occurred.
 */
/* @[define_transportwritev] */
typedef int32_t ( * TransportWritev_t )( NetworkContext_t * pNetworkContext,
                                         TransportOutVector_t * pIoVec,
                                         size_t ioVecCount );
/* @[define_transportwritev] */

/**
 * @transportstruct
 * @brief The transport layer interface.
 */
/* @[define_transportinterface] */
typedef struct TransportInterface
{
    TransportRecv_t recv;               /**< Transport receive function pointer. */
    TransportSend_t send;               /**< Transport send function pointer. */
    TransportWritev_t writev;           /**< Transport writev function pointer. */
    NetworkContext_t * pNetworkContext; /**< Implementation-defined network context. */
} TransportInterface_t;
/* @[define_transportinterface] */

/* *INDENT-OFF* */
#ifdef __cplusplus
    }
#endif
/* *INDENT-ON* */

#endif /* ifndef TRANSPORT_INTERFACE_H_ */
