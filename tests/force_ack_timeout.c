/* Test-only public API fault injection. Team/Codex, 2026-11-05.
 * Used only in explicitly selected game children, never normal launch.
 * This simulates an ACK timeout; it is not evidence of native ACK behavior.
 */
#include <dds/dds.h>
dds_return_t dds_wait_for_acks(dds_entity_t writer, dds_duration_t timeout)
{
    (void)writer; (void)timeout;
    return DDS_RETCODE_TIMEOUT;
}
