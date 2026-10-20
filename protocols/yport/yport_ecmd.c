/*
 *
 * Copyright (c) 2010 by Erik Kunze <ethersex@erik-kunze.de>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License (either version 2 or
 * version 3) as published by the Free Software Foundation.
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

#include <string.h>
#include <avr/pgmspace.h>

#include "config.h"
#include "core/bit-macros.h"
#include "core/debug.h"
#include "protocols/yport/yport.h"

#include "protocols/ecmd/ecmd-base.h"


int16_t
parse_cmd_yport_stats(char *cmd, char *output, uint16_t len)
{
  int16_t chars = snprintf_P(output, len,
                             PSTR("rx fe=%u, ov=%u, pe=%u, bf=%u, rt=%u"),
                             yport_rx_frameerror,
                             yport_rx_overflow,
                             yport_rx_parityerror,
                             yport_rx_bufferfull,
                             yport_eth_retransmit);
  return ECMD_FINAL(chars);
}

#ifdef DEBUG_YPORT
int16_t
parse_cmd_yport_probe(char *cmd, char *output, uint16_t len)
{
  /* The ECMD output buffer is small (ECMD_OUTPUTBUF_LENGTH, typically 50
     bytes), so the diagnostics are dumped across several ECMD_AGAIN()
     continuation lines.  The call index is kept in cmd[1]. */
  if (cmd[0] != ECMD_STATE_MAGIC)
  {
    cmd[0] = ECMD_STATE_MAGIC;
    cmd[1] = 0;
  }

  struct yport_probe_t p;
  yport_probe_get(&p);

  switch (cmd[1]++)
  {
    case 0:
      return ECMD_AGAIN(snprintf_P(output, len,
          PSTR("rst=%u fl=%02x path=%u"),
          p.reset_count, p.flags, p.last_path));
    case 1:
      return ECMD_AGAIN(snprintf_P(output, len,
          PSTR("rlen=%u rsent=%u sgtl=%u"),
          p.recv_len_max, p.recv_sent_max, p.sent_gt_len));
    case 2:
      return ECMD_AGAIN(snprintf_P(output, len,
          PSTR("lgtb=%u snd=%u av=%u"),
          p.len_gt_buf, p.sendlen_max, p.avail_max));
    case 3:
      return ECMD_AGAIN(snprintf_P(output, len,
          PSTR("drn=%u ovf=%u rjct=%u"),
          p.drain_max, p.rx_overflow, p.rxstart_reject));
    default:
      return ECMD_FINAL_OK;
  }
}

int16_t
parse_cmd_yport_probe_clear(char *cmd, char *output, uint16_t len)
{
  yport_probe_clear();
  return ECMD_FINAL(snprintf_P(output, len, PSTR("probe cleared")));
}
#endif

/*
  -- Ethersex META --
  block([[YPORT]] commands)
  ecmd_ifdef(DEBUG_YPORT)
    ecmd_feature(yport_stats, "yport stats",, Report statistic counters)
    dnl 'clear' must be registered before 'probe' because the ECMD dispatcher
    dnl matches commands by first-prefix-match.
    ecmd_feature(yport_probe_clear, "yport probe clear",, Reset EEPROM diagnostics)
    ecmd_feature(yport_probe, "yport probe",, Dump EEPROM diagnostics)
  ecmd_endif()
*/
