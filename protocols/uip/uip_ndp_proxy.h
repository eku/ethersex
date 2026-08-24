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

#ifndef UIP_NDP_PROXY_H
#define UIP_NDP_PROXY_H

#include "uip.h"
#include "uipopt.h"

/**
 * \file
 *         IPv6 Neighbor Discovery Protocol Proxy
 * \author
 *         Ethersex Project
 */

/**
 * Check if the given IPv6 address should be proxied
 *
 * \param target_addr The IPv6 address being solicited
 * \return true if we should proxy for this address, false otherwise
 */
bool ndp_proxy_check(uip_ipaddr_t *target_addr);

#endif /* UIP_NDP_PROXY_H */
