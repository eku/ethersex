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

#include "config.h"
#include "uip_ndp_proxy.h"
#include "uip.h"
#include "uipopt.h"

#ifdef ETHERNET_SUPPORT

/*
 * Check if we should proxy for the given IPv6 address
 * Returns true if the address matches any configured proxy address
 */
bool
ndp_proxy_check(uip_ipaddr_t *target_addr)
{
#ifdef RFM12_NDP_PROXY
  if(uip_ipaddr_prefixlencmp(target_addr, &rfm12_stack_hostaddr,
                             CONF_RFM12_IP6_PREFIX_LEN) == 0) {
    return true;
  }
#endif
#ifdef ZBUS_NDP_PROXY
  if(uip_ipaddr_prefixlencmp(target_addr, &zbus_stack_hostaddr,
                             CONF_ZBUS_IP6_PREFIX_LEN) == 0) {
    return true;
  }
#endif
#ifdef USB_NDP_PROXY
  if(uip_ipaddr_prefixlencmp(target_addr, &usb_stack_hostaddr,
                             CONF_USB_NET_IP6_PREFIX_LEN) == 0) {
    return true;
  }
#endif
  return false;
}

#endif /* ETHERNET_SUPPORT */
