/*
 *
 * Copyright (c) 2008 by Christian Dietrich <stettberger@dokucode.de>
 * Copyright (c) 2010 by Erik Kunze <ethersex@erik-kunze.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * For more information on the GPL, please go to:
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <util/atomic.h>
#include <string.h>
#include "core/eeprom.h"

#define YPORT_USART_CONFIG_8N1 1
#define YPORT_USART_CONFIG_8E2 2

#include "config.h"
#include "yport.h"
#include "yport_net.h"


#define USE_USART YPORT_USE_USART
#define BAUD YPORT_BAUDRATE
#include "core/usart.h"

/* We generate our own usart init module, for our usart port */
#if YPORT_USART_CONFIG == YPORT_USART_CONFIG_8N1
	generate_usart_init()
#elif YPORT_USART_CONFIG == YPORT_USART_CONFIG_8E2
	generate_usart_init_8E2()
#else
	#error "YPORT_USART_CONFIG not correctly defined"
#endif

struct yport_buffer yport_send_buffer;
struct yport_buffer yport_recv_buffer;
#ifdef DEBUG_YPORT
uint16_t yport_rx_frameerror;
uint16_t yport_rx_overflow;
uint16_t yport_rx_parityerror;
uint16_t yport_rx_bufferfull;
uint16_t yport_eth_retransmit;
#endif
#if CONF_YPORT_FLUSH > 0
uint8_t yport_lf;
#endif

void yport_init(void)
{
  usart_init();
}

uint8_t
yport_rxstart(uint8_t * data, uint16_t len)
{
  uint8_t rc = 0;

  /* Synchronise with the TX interrupt handler, which drains
     yport_send_buffer concurrently.  Without this the ISR could leave
     .sent greater than .len, which made the .diff calculation below
     underflow and a following memmove() of ~65535 bytes corrupt RAM and
     freeze the firmware under heavy TCP->USART load. */
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
  {
    uint16_t sent = yport_send_buffer.sent;
    uint16_t buflen = yport_send_buffer.len;

    /* Sanitise a stale/inconsistent state: never allow sent > len, as
       that makes the length arithmetic below wrap around. */
    if (sent > buflen)
      buflen = sent;

    uint16_t diff = buflen - sent;
    if (diff == 0 || (diff + len) < YPORT_BUFFER_LEN)
    {
      if (diff != 0)
      {
        /* move the not-yet-transmitted bytes to the front of the buffer */
        memmove(yport_send_buffer.data,
                yport_send_buffer.data + sent, diff);
      }
      memcpy(yport_send_buffer.data + diff, data, len);
      yport_send_buffer.len = diff + len;

      /* Enable the tx interrupt and send the first character */
      yport_send_buffer.sent = 1;
      usart(UCSR, B) |= _BV(usart(TXCIE));
      usart(UDR) = yport_send_buffer.data[0];
      rc = 1;
    }
  }
  return rc;
}


ISR(usart(USART, _TX_vect))
{
  if (yport_send_buffer.sent < yport_send_buffer.len)
  {
    usart(UDR) = yport_send_buffer.data[yport_send_buffer.sent++];
  }
  else
  {
    /* Disable this interrupt */
    usart(UCSR, B) &= ~(_BV(usart(TXCIE)));
  }
}

ISR(usart(USART, _RX_vect))
{
  while (usart(UCSR, A) & _BV(usart(RXC)))
  {
    if (usart(UCSR, A) & (_BV(usart(FE)) | _BV(usart(DOR)) | _BV(usart(UPE))))
    {
#ifdef DEBUG_YPORT
      if (usart(UCSR, A) & _BV(usart(FE)))
        yport_rx_frameerror++;
      if (usart(UCSR, A) & _BV(usart(DOR)))
        yport_rx_overflow++;
      if (usart(UCSR, A) & _BV(usart(UPE)))
        yport_rx_parityerror++;
#endif
      uint8_t v = usart(UDR);
      (void) v;
    }
    else
    {
      uint8_t v = usart(UDR);
      if (yport_recv_buffer.len < YPORT_BUFFER_LEN)
        yport_recv_buffer.data[yport_recv_buffer.len++] = v;
#ifdef DEBUG_YPORT
      else
        yport_rx_bufferfull++;
#endif
#if CONF_YPORT_FLUSH > 0
      if (v == 0x0A)
        yport_lf = 1;
#endif
    }
  }
}

/*
  -- Ethersex META --
  dnl yport_init call must be done after network_init (according to earlier
  dnl comments.  Therefore we initialize via net_init and control the
  dnl order via the Makefile.
  
  header(protocols/yport/yport.h)

  net_init(yport_init)
*/
