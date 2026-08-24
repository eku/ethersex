# IPv4/IPv6 Dual-Stack Proof-of-Concept

## Issue
Currently Ethersex mutually excludes IPv4 and IPv6 support. When IPv6 is enabled,
IPv4 is automatically disabled (see config.in lines 226-229). This prevents the
bridge/router from forwarding both IPv4 and IPv6 packets.

## Goal
Enable simultaneous IPv4 and IPv6 support (dual-stack) to allow the bridge to
forward both IPv4 and IPv6 packets between network interfaces.

## Changes Made (Proof-of-Concept)

### 1. config.in
- Removed mutual exclusion between IPv4 and IPv6
- Now both `IPV4_SUPPORT` and `IPV6_SUPPORT` can be enabled independently

### 2. protocols/uip/config.in
- Added `DUAL_STACK_SUPPORT` configuration option
- Enabled when both IPV4_SUPPORT and IPV6_SUPPORT are set

### 3. protocols/uip/uip-conf.h
- Added `UIP_CONF_DUAL_STACK` define based on `DUAL_STACK_SUPPORT`

### 4. hardware/ethernet/enc28j60_process.c
- Modified to handle both Ethernet types (0x0800 for IPv4, 0x86DD for IPv6)
- When DUAL_STACK_SUPPORT is enabled, processes both packet types

## Remaining Work

### Core uIP Structure Changes
The `uip_tcpip_hdr` structure in uip.h is conditionally compiled:
- If IPv6: Contains IPv6 header fields
- If IPv4: Contains IPv4 header fields

For dual-stack, we need to:
1. Support both header types in the same structure
2. Add version detection (IPv4: version=4, IPv6: version=6)
3. Update all code that accesses header fields to use the correct fields

### Router Changes
The `router_find_stack()` function needs to:
1. Detect IP version from packet header
2. Use appropriate address comparison (32-bit for IPv4, 128-bit for IPv6)
3. Handle both address types in the same code path

### Address Resolution
- ARP for IPv4
- NDP for IPv6
Both need to work simultaneously

### Buffer Layout
Different link layers have different LLH (Link Layer Header) lengths:
- Ethernet: 14 bytes
- RFM12: 2 bytes
- ZBUS: 0 bytes
- USB: 0 bytes

The bridge mechanism uses offsets to handle this, but dual-stack adds complexity.

## Testing

To test the proof-of-concept:
1. Enable `IPV4_SUPPORT=y`
2. Enable `IPV6_SUPPORT=y`
3. Enable `DUAL_STACK_SUPPORT=y`
4. Enable `ROUTER_SUPPORT=y`
5. Enable `IP_FORWARDING_SUPPORT=y`
6. Configure IPv4 and IPv6 addresses on multiple stacks (ENC, RFM12, ZBUS)
7. Send both IPv4 and IPv6 packets between stacks

## Notes

This is a proof-of-concept demonstrating the configuration changes needed
for dual-stack support. Full implementation requires significant changes to
the uIP stack to support both IPv4 and IPv6 simultaneously.

The current approach maintains backward compatibility:
- If only IPv4 is enabled: Works as before
- If only IPv6 is enabled: Works as before
- If both are enabled with DUAL_STACK_SUPPORT: Enables dual-stack mode

## References
- Issue #299: https://github.com/ethersex/ethersex/issues/299
- RFC 4213: Basic Transition Mechanisms for IPv6 Hosts and Routers
