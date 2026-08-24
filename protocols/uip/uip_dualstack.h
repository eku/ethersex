/*
 * Copyright (c) 2024 Ethersex Project
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License (version 3)
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef UIP_DUALSTACK_H
#define UIP_DUALSTACK_H

#include "uip.h"
#include "uipopt.h"

/**
 * \file
 *         Dual-stack support functions for IPv4/IPv6 coexistence
 * \author
 *         Ethersex Project
 */

#ifdef UIP_CONF_DUAL_STACK

/**
 * Get the IP version from the first byte of the IP header
 * \param buf Pointer to the IP header
 * \return 4 for IPv4, 6 for IPv6
 */
static inline uint8_t
uip_ip_version(const struct uip_tcpip_hdr *buf)
{
  return (buf->ip.v4.vhl >> 4);
}

/**
 * Check if the IP header is IPv4
 * \param buf Pointer to the IP header
 * \return true if IPv4, false otherwise
 */
static inline bool
uip_is_ipv4(const struct uip_tcpip_hdr *buf)
{
  return uip_ip_version(buf) == 4;
}

/**
 * Check if the IP header is IPv6
 * \param buf Pointer to the IP header
 * \return true if IPv6, false otherwise
 */
static inline bool
uip_is_ipv6(const struct uip_tcpip_hdr *buf)
{
  return uip_ip_version(buf) == 6;
}

/**
 * Get the protocol field from IP header (works for both IPv4 and IPv6)
 * \param buf Pointer to the IP header
 * \return The protocol value
 */
static inline uint8_t
uip_get_proto(const struct uip_tcpip_hdr *buf)
{
  if (uip_is_ipv4(buf)) {
    return buf->ip.v4.proto;
  } else {
    return buf->ip.v6.proto;
  }
}

/**
 * Get the TTL/Hop Limit from IP header
 * \param buf Pointer to the IP header
 * \return The TTL value
 */
static inline uint8_t
uip_get_ttl(const struct uip_tcpip_hdr *buf)
{
  if (uip_is_ipv4(buf)) {
    return buf->ip.v4.ttl;
  } else {
    return buf->ip.v6.ttl;
  }
}

/**
 * Decrement the TTL/Hop Limit in the IP header
 * \param buf Pointer to the IP header
 */
static inline void
uip_decr_ttl(struct uip_tcpip_hdr *buf)
{
  if (uip_is_ipv4(buf)) {
    buf->ip.v4.ttl--;
  } else {
    buf->ip.v6.ttl--;
  }
}

/**
 * Get the destination IP address from IP header
 * \param buf Pointer to the IP header
 * \return Pointer to the destination address
 */
static inline uip_ipaddr_t *
uip_get_destipaddr(const struct uip_tcpip_hdr *buf)
{
  if (uip_is_ipv4(buf)) {
    return (uip_ipaddr_t *)buf->ip.v4.destipaddr;
  } else {
    return (uip_ipaddr_t *)buf->ip.v6.destipaddr;
  }
}

/**
 * Get the source IP address from IP header
 * \param buf Pointer to the IP header
 * \return Pointer to the source address
 */
static inline uip_ipaddr_t *
uip_get_srcipaddr(const struct uip_tcpip_hdr *buf)
{
  if (uip_is_ipv4(buf)) {
    return (uip_ipaddr_t *)buf->ip.v4.srcipaddr;
  } else {
    return (uip_ipaddr_t *)buf->ip.v6.srcipaddr;
  }
}

/**
 * Set the destination IP address in IP header
 * \param buf Pointer to the IP header
 * \param addr Pointer to the address to set
 */
static inline void
uip_set_destipaddr(struct uip_tcpip_hdr *buf, const uip_ipaddr_t *addr)
{
  if (uip_is_ipv4(buf)) {
    ((u16_t *)buf->ip.v4.destipaddr)[0] = ((u16_t *)addr)[0];
    ((u16_t *)buf->ip.v4.destipaddr)[1] = ((u16_t *)addr)[1];
  } else {
    memcpy(buf->ip.v6.destipaddr, addr, sizeof(uip_ip6addr_t));
  }
}

/**
 * Set the source IP address in IP header
 * \param buf Pointer to the IP header
 * \param addr Pointer to the address to set
 */
static inline void
uip_set_srcipaddr(struct uip_tcpip_hdr *buf, const uip_ipaddr_t *addr)
{
  if (uip_is_ipv4(buf)) {
    ((u16_t *)buf->ip.v4.srcipaddr)[0] = ((u16_t *)addr)[0];
    ((u16_t *)buf->ip.v4.srcipaddr)[1] = ((u16_t *)addr)[1];
  } else {
    memcpy(buf->ip.v6.srcipaddr, addr, sizeof(uip_ip6addr_t));
  }
}

/**
 * Get the IP payload length from IP header
 * \param buf Pointer to the IP header
 * \return The payload length
 */
static inline uint16_t
uip_get_ip_len(const struct uip_tcpip_hdr *buf)
{
  if (uip_is_ipv4(buf)) {
    return (uint16_t)((buf->ip.v4.len[0] << 8) | buf->ip.v4.len[1]);
  } else {
    return (uint16_t)((buf->ip.v6.len[0] << 8) | buf->ip.v6.len[1]);
  }
}

/**
 * Get the IP header length (for IPv4) or 40 (for IPv6)
 * \param buf Pointer to the IP header
 * \return The IP header length in bytes
 */
static inline uint8_t
uip_get_ip_hdr_len(const struct uip_tcpip_hdr *buf)
{
  if (uip_is_ipv4(buf)) {
    return (buf->ip.v4.vhl & 0x0F) * 4;
  } else {
    return 40; /* IPv6 header is always 40 bytes */
  }
}

#endif /* UIP_CONF_DUAL_STACK */

#endif /* UIP_DUALSTACK_H */
