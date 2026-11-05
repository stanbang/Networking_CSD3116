/**
 * @file
 * @brief Creates bounded DDS reader and writer endpoints with topic-specific QoS.
 *
 * Team contribution split: Stanley Lur 50%, Jordain Ng 50%.
 * Project AI-use disclosure: docs/AI_USAGE.md.
 */
#include <stdio.h>
#include <string.h>
#include "common/protocol.h"

/* Safe diagnostics without logging pointers or wire payloads. */
bool dds_ok(dds_return_t int_result, const char *str_operation)
{
    if (int_result >= 0) return true;
    fprintf(stderr, "%s: %s\n", str_operation, dds_strretcode(int_result));
    return false;
}

/* Exact topics enable P2 per-identity permissions. Queues are always bounded. */
dds_entity_t endpoint(dds_entity_t int_participant, const dds_topic_descriptor_t *str_desc,
                      const char *str_kind, unsigned int int_identity, bool b_writer, bool b_reliable)
{
    char str_name[96] = {0};
    int int_len = snprintf(str_name, sizeof(str_name), "DDSAsteroids_%s_%u", str_kind, int_identity);
    if (int_len < 0 || (size_t)int_len >= sizeof(str_name)) return -1;
    dds_entity_t int_topic = dds_create_topic(int_participant, str_desc, str_name, NULL, NULL);
    if (!dds_ok(int_topic, "create_topic")) return int_topic;
    dds_qos_t *str_qos = dds_create_qos();
    if (!str_qos) return -1;
    dds_qset_reliability(str_qos, b_reliable ? DDS_RELIABILITY_RELIABLE : DDS_RELIABILITY_BEST_EFFORT,
                         DDS_MSECS(100));
    /* Join retries are idempotent: retain the latest response rather than let
     * a non-reading authenticated peer block every other identity's worker. */
    bool b_all = b_reliable && strcmp(str_kind, "Response") != 0 &&
                 strcmp(str_kind, "AccountRequest") != 0 && strcmp(str_kind, "AccountResponse") != 0;
    dds_qset_history(str_qos, b_all ? DDS_HISTORY_KEEP_ALL : DDS_HISTORY_KEEP_LAST, 1);
    dds_qset_resource_limits(str_qos, b_all ? 32 : 1, 1, b_all ? 32 : 1);
    dds_qset_liveliness(str_qos, DDS_LIVELINESS_AUTOMATIC, DDS_SECS(3));
    dds_entity_t int_endpoint = b_writer ? dds_create_writer(int_participant, int_topic, str_qos, NULL) :
                                         dds_create_reader(int_participant, int_topic, str_qos, NULL);
    dds_delete_qos(str_qos);
    dds_ok(int_endpoint, "create_endpoint");
    return int_endpoint;
}
