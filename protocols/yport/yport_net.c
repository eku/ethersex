/*
 *
 * Copyright (c) 2008 by Christian Dietrich <stettberger@dokucode.de>
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
#include <util/atomic.h>
#include <string.h>

#include "config.h"
#include "yport_net.h"
#include "protocols/uip/uip.h"
#include "core/debug.h"
#include "yport.h"

static uip_conn_t *yport_conn = NULL;
#if CONF_YPORT_FLUSH > 0
static uint8_t yport_lastservice;
#endif

void
yport_net_init(void)
{
  uip_listen(HTONS(YPORT_PORT), yport_net_main);
}

void
yport_net_main(void)
{
#if CONF_YPORT_FLUSH > 0
  yport_lastservice++;
#endif

#ifdef DEBUG_YPORT
  yport_probe_poll();
#endif

  if (uip_connected())
  {
    if (yport_conn == NULL)
    {
      yport_conn = uip_conn;
      uip_conn->wnd = YPORT_BUFFER_LEN - 1;
      /* delete the receive buffer */
      yport_recv_buffer.len = 0;
    }
    else
    {
      /* if we already have a connection, send an error */
      memcpy_P (uip_sappdata, PSTR("ERROR: connection blocked\n"), 27);
      uip_send (uip_sappdata, 27);
    }
  }

  else if (uip_closed() || uip_aborted() || uip_timedout())
  {
    /* if the closed connection was our connection, clean yport_conn */
    if (yport_conn == uip_conn)
    {
      yport_conn = NULL;
      return;
    }
  }

  else if (uip_acked())
  {
    /* if the peer is not our connection, close it */
    if (yport_conn != uip_conn)
      uip_close();
    else
    {
      /* data we have sent was acked */
#ifdef DEBUG_YPORT
      uint8_t probe_anomaly = 0;
#endif
      ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
      {
        uint16_t sent = yport_recv_buffer.sent;
        uint16_t len = yport_recv_buffer.len;
#ifdef DEBUG_YPORT
        yport_probe_ram.last_path = YPORT_PROBE_PATH_DRAIN;
        if (len > yport_probe_ram.recv_len_max)
          yport_probe_ram.recv_len_max = len;
        if (sent > yport_probe_ram.recv_sent_max)
          yport_probe_ram.recv_sent_max = sent;
        if (sent > len)
        {
          yport_probe_ram.sent_gt_len++;
          yport_probe_ram.flags |= YPORT_PROBE_FLAG_SENT_GT_LEN;
          probe_anomaly = 1;
          sent = len;
        }
        if (len > YPORT_BUFFER_LEN)
        {
          yport_probe_ram.len_gt_buf++;
          yport_probe_ram.flags |= YPORT_PROBE_FLAG_LEN_GT_BUF;
          probe_anomaly = 1;
        }
#endif
        /* defensive: never let the subtraction underflow */
        if (sent > len)
          sent = len;
        yport_recv_buffer.len = len - sent;
        memmove(yport_recv_buffer.data,
                yport_recv_buffer.data + sent,
                len - sent);
        yport_recv_buffer.sent = 0;
#ifdef DEBUG_YPORT
        if ((len - sent) > yport_probe_ram.drain_max)
          yport_probe_ram.drain_max = len - sent;
#endif
      }
#ifdef DEBUG_YPORT
      if (probe_anomaly)
        yport_probe_write();
#endif
    }
  }

  if (yport_conn != uip_conn)
    return;

  if (uip_newdata())
  {
    if (uip_len <= YPORT_BUFFER_LEN &&
        yport_rxstart(uip_appdata, uip_len) != 0)
    {
      /* prevent the other side from sending more data via tcp */
      uip_stop();
    }
  }

  /* retransmit last packet */
  if (uip_rexmit())
  {
#ifdef DEBUG_YPORT
    uint8_t probe_anomaly = 0;
#endif
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
#ifdef DEBUG_YPORT
      yport_probe_ram.last_path = YPORT_PROBE_PATH_REXMIT;
#endif
      uint16_t rl = yport_recv_buffer.sent;
#ifdef DEBUG_YPORT
      if (rl > yport_probe_ram.recv_sent_max)
        yport_probe_ram.recv_sent_max = rl;
      if (rl > UIP_TCP_MSS)
      {
        yport_probe_ram.flags |= YPORT_PROBE_FLAG_SEND_GT_MSS;
        rl = UIP_TCP_MSS;
        probe_anomaly = 1;
      }
#endif
      uip_send(yport_recv_buffer.data, rl);
    }
#ifdef DEBUG_YPORT
    yport_eth_retransmit++;
    if (probe_anomaly)
      yport_probe_write();
#endif
  }

  /* restart connection */
  if (uip_poll() && uip_stopped(yport_conn))
  {
    uint8_t empty;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
      empty = (yport_send_buffer.sent == yport_send_buffer.len);
    }
    if (empty)
      uip_restart();
  }

  /* send data */
  if ((uip_poll() || uip_acked())
      /* only hand new data to uIP when nothing is still in flight */
      && !uip_outstanding(yport_conn)
      /* receive buffer reached water mark */
#if CONF_YPORT_FLUSH > 0
      && (yport_recv_buffer.len > (YPORT_BUFFER_LEN / 4)
          /* last transmission is at least one second ago */
          || yport_lastservice >= CONF_YPORT_FLUSH
          /* we received a linefeed character, send immediately */
          || yport_lf)
#endif
    )
  {
    /* we have enough uart data, send it via tcp */
#ifdef DEBUG_YPORT
    uint8_t probe_anomaly = 0;
#endif
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
      uint16_t avail = yport_recv_buffer.len;
#ifdef DEBUG_YPORT
      yport_probe_ram.last_path = YPORT_PROBE_PATH_SEND;
      if (avail > yport_probe_ram.avail_max)
        yport_probe_ram.avail_max = avail;
      if (avail > yport_probe_ram.recv_len_max)
        yport_probe_ram.recv_len_max = avail;
      if (avail > YPORT_BUFFER_LEN)
      {
        yport_probe_ram.len_gt_buf++;
        yport_probe_ram.flags |= YPORT_PROBE_FLAG_AVAIL_GT_BUF;
        probe_anomaly = 1;
      }
#endif
      /* Never hand uIP more than it can transmit in a single segment.
         uip_send() silently drops everything beyond the peer's advertised
         window / MSS, while we would otherwise mark it as sent and drain
         it on the ACK, losing those bytes forever.  By sending only what
         fits, the remainder stays buffered and is sent on the next
         poll/ack. */
      uint16_t sendlen = (avail < yport_conn->mss) ? avail : yport_conn->mss;
#ifdef DEBUG_YPORT
      if (sendlen > yport_probe_ram.sendlen_max)
        yport_probe_ram.sendlen_max = sendlen;
      /* Hard safety clamp so uip_send() can never overrun uip_sappdata
         even if yport_conn->mss got corrupted. */
      if (sendlen > UIP_TCP_MSS)
      {
        yport_probe_ram.flags |= YPORT_PROBE_FLAG_SEND_GT_MSS;
        sendlen = UIP_TCP_MSS;
        probe_anomaly = 1;
      }
#endif

      uip_send(yport_recv_buffer.data, sendlen);
      yport_recv_buffer.sent = sendlen;
#if CONF_YPORT_FLUSH > 0
      yport_lastservice = 0;
      yport_lf = 0;
#endif
    }
#ifdef DEBUG_YPORT
    if (probe_anomaly)
      yport_probe_write();
#endif
  }
}

#if CONF_YPORT_FLUSH > 0
void
yport_net_periodic(void)
{
  if (yport_conn)
  {
    uip_stack_set_active(yport_conn->stack);
    uip_conn = yport_conn;
    uip_process(UIP_TIMER);
    if (uip_len > 0)
      router_output();
  }
}
#endif

/*
  -- Ethersex META --
  header(protocols/yport/yport_net.h)
  net_init(yport_net_init)
  ifdef(`conf_YPORT_FLUSH',`timer(value_YPORT_FLUSH, yport_net_periodic())')
*/
