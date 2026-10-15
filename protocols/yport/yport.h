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

#ifndef _YPORT_H
#define _YPORT_H


struct yport_buffer
{
  uint16_t len;
  uint16_t sent;
  uint8_t data[YPORT_BUFFER_LEN];
};

void yport_init(void);
uint8_t yport_rxstart(uint8_t * data, uint16_t len);

extern struct yport_buffer yport_send_buffer;
extern struct yport_buffer yport_recv_buffer;
#ifdef DEBUG_YPORT
extern uint16_t yport_rx_frameerror;
extern uint16_t yport_rx_overflow;
extern uint16_t yport_rx_parityerror;
extern uint16_t yport_rx_bufferfull;
extern uint16_t yport_eth_retransmit;

/* ---- EEPROM-persistent diagnostics (survives a firmware reboot) ---- */
#define YPORT_PROBE_MAGIC 0x5A

struct yport_probe_t
{
  uint8_t  magic;             /* YPORT_PROBE_MAGIC while probes active */
  uint8_t  flags;             /* anomaly bits, see YPORT_PROBE_FLAG_* */
  uint16_t reset_count;       /* # boots without a clean shutdown */
  uint16_t recv_len_max;      /* max yport_recv_buffer.len observed */
  uint16_t recv_sent_max;     /* max yport_recv_buffer.sent observed */
  uint16_t sent_gt_len;       /* # times sent > len (underflow) */
  uint16_t len_gt_buf;        /* # times len > YPORT_BUFFER_LEN (corruption) */
  uint16_t sendlen_max;       /* max sendlen handed to uip_send */
  uint16_t avail_max;         /* max avail in the send block */
  uint16_t drain_max;         /* max drain memmove count (len - sent) */
  uint16_t rx_overflow;       /* USART RX DOR (hardware overrun) count */
  uint16_t rxstart_reject;    /* # times TCP->USART buffer was full (rxstart 0) */
  uint8_t  last_path;         /* last instrumented code path */
};

#define YPORT_PROBE_FLAG_SENT_GT_LEN   0x01
#define YPORT_PROBE_FLAG_LEN_GT_BUF    0x02
#define YPORT_PROBE_FLAG_SEND_GT_MSS   0x04
#define YPORT_PROBE_FLAG_AVAIL_GT_BUF  0x08
#define YPORT_PROBE_FLAG_RX_OVERFLOW   0x10

#define YPORT_PROBE_PATH_IDLE    0
#define YPORT_PROBE_PATH_DRAIN   1
#define YPORT_PROBE_PATH_SEND    2
#define YPORT_PROBE_PATH_RXSTART 3
#define YPORT_PROBE_PATH_REXMIT  4
#define YPORT_PROBE_PATH_RX_VECT 5
#define YPORT_PROBE_PATH_TX_VECT 6

extern struct yport_probe_t yport_probe_ram;
extern volatile uint8_t yport_probe_dirty;

void yport_probe_init(void);
void yport_probe_poll(void);
void yport_probe_write(void);
void yport_probe_get(struct yport_probe_t *dst);
void yport_probe_clear(void);
#endif
#if CONF_YPORT_FLUSH > 0
extern uint8_t yport_lf;
#endif


#endif /* _YPORT_H */
