/* Native PKI-DH, signed policies and AES-GCM. Author: <team>, Codex assistance. Date: 2026-11-03. */
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dds/security/dds_security_api_defs.h>
#include <dds/ddsrt/log.h>
#include "common/security.h"
#ifndef AST_PLUGIN_DIR
#define AST_PLUGIN_DIR "/usr/lib/x86_64-linux-gnu/libddsc0debian/"
#endif

/* Clients need no raw native trace file, even with the server tracing config. */
static void discard_trace(void *ptr_unused, const dds_log_data_t *str_unused)
{
    (void)ptr_unused; (void)str_unused;
}

/* Registered before DDS threads exist and reset only after participant deletion. */
void client_trace_sink(bool b_install) { dds_set_trace_sink(b_install ? discard_trace : NULL, NULL); }

/* Private Linux storage is mandatory: drvfs chmod may leave files at mode777. */
static bool private_directory(const char *str_path, char str_real[PATH_MAX])
{
    struct stat str_stat = {0};
    if (!str_path || !realpath(str_path, str_real) || lstat(str_path, &str_stat) != 0 ||
        !S_ISDIR(str_stat.st_mode) || str_stat.st_uid != geteuid() || (str_stat.st_mode & 0777) != 0700) {
        fprintf(stderr, "Security directory must be owned by this user, nonsymlink, mode0700\n");
        return false;
    }
    return true;
}

/* Check ownership and permissions before the plugin opens any identity material. */
static bool file_property(dds_qos_t *str_qos, const char *str_root, const char *str_relative,
                           const char *str_property, bool b_private)
{
    char str_path[PATH_MAX], str_uri[PATH_MAX + 6];
    struct stat str_stat = {0};
    int int_length = snprintf(str_path, sizeof(str_path), "%s/%s", str_root, str_relative);
    if (int_length < 0 || (size_t)int_length >= sizeof(str_path) || lstat(str_path, &str_stat) != 0 ||
        !S_ISREG(str_stat.st_mode) || str_stat.st_uid != geteuid() ||
        (str_stat.st_mode & 0022) || (b_private && (str_stat.st_mode & 0777) != 0600)) {
        fprintf(stderr, "Security material missing or unsafe: %s\n", str_relative); return false;
    }
    int_length = snprintf(str_uri, sizeof(str_uri), "file:%s", str_path);
    if (int_length < 0 || (size_t)int_length >= sizeof(str_uri)) return false;
    dds_qset_prop(str_qos, str_property, str_uri);
    return true;
}

/* Property names come from installed DDS Security headers, entry points from
 * official Cyclone0.10 documentation, verified against installed library symbols. */
static void plugins(dds_qos_t *str_qos)
{
    dds_qset_prop(str_qos, DDS_SEC_PROP_AUTH_LIBRARY_PATH, AST_PLUGIN_DIR "libdds_security_auth.so");
    dds_qset_prop(str_qos, DDS_SEC_PROP_AUTH_LIBRARY_INIT, "init_authentication");
    dds_qset_prop(str_qos, DDS_SEC_PROP_AUTH_LIBRARY_FINALIZE, "finalize_authentication");
    dds_qset_prop(str_qos, DDS_SEC_PROP_ACCESS_LIBRARY_PATH, AST_PLUGIN_DIR "libdds_security_ac.so");
    dds_qset_prop(str_qos, DDS_SEC_PROP_ACCESS_LIBRARY_INIT, "init_access_control");
    dds_qset_prop(str_qos, DDS_SEC_PROP_ACCESS_LIBRARY_FINALIZE, "finalize_access_control");
    dds_qset_prop(str_qos, DDS_SEC_PROP_CRYPTO_LIBRARY_PATH, AST_PLUGIN_DIR "libdds_security_crypto.so");
    dds_qset_prop(str_qos, DDS_SEC_PROP_CRYPTO_LIBRARY_INIT, "init_crypto");
    dds_qset_prop(str_qos, DDS_SEC_PROP_CRYPTO_LIBRARY_FINALIZE, "finalize_crypto");
}

/* Every normal participant has explicit security properties; missing assets fail closed. */
dds_entity_t secure_participant(const Options *str_options, bool b_server)
{
    char str_root[PATH_MAX], str_name[32], str_relative[96];
    if (!private_directory(str_options->str_security_dir, str_root)) return -1;
    if (!getenv("CYCLONEDDS_URI")) {
        char str_config[PATH_MAX], str_uri[PATH_MAX + 8];
        if (!realpath("config/cyclonedds.xml", str_config)) return -1;
        int int_length = snprintf(str_uri, sizeof(str_uri), "file://%s", str_config);
        if (int_length < 0 || (size_t)int_length >= sizeof(str_uri) || setenv("CYCLONEDDS_URI", str_uri, 0) != 0) return -1;
    }
    if (!b_server && strcmp(str_options->str_fault, "no-cert") == 0)
        return dds_create_participant(AST_DOMAIN, NULL, NULL); /* Explicit negative-test peer only. */
    bool b_rogue = !b_server && strcmp(str_options->str_fault, "wrong-ca") == 0;
    snprintf(str_name, sizeof(str_name), b_server ? "server" : "client%u", str_options->int_identity);
    dds_qos_t *str_qos = dds_create_qos();
    if (!str_qos) return -1;
    plugins(str_qos);
    bool b_ok = file_property(str_qos, str_root, b_rogue ? "rogue_ca/ca.crt" : "identity_ca/ca.crt", DDS_SEC_PROP_AUTH_IDENTITY_CA, false) &&
        file_property(str_qos, str_root, "permissions_ca/ca.crt", DDS_SEC_PROP_ACCESS_PERMISSIONS_CA, false) &&
        file_property(str_qos, str_root, "governance.p7s", DDS_SEC_PROP_ACCESS_GOVERNANCE, false);
    snprintf(str_relative, sizeof(str_relative), "identities/%s%s.crt", b_rogue ? "rogue_" : "", str_name);
    b_ok = b_ok && file_property(str_qos, str_root, str_relative, DDS_SEC_PROP_AUTH_IDENTITY_CERT, false);
    snprintf(str_relative, sizeof(str_relative), "identities/%s%s.key", b_rogue ? "rogue_" : "", str_name);
    b_ok = b_ok && file_property(str_qos, str_root, str_relative, DDS_SEC_PROP_AUTH_PRIV_KEY, true);
    snprintf(str_relative, sizeof(str_relative), "permissions/%s.p7s", str_name);
    b_ok = b_ok && file_property(str_qos, str_root, str_relative, DDS_SEC_PROP_ACCESS_PERMISSIONS, false);
    dds_entity_t int_participant = b_ok ? dds_create_participant(AST_DOMAIN, str_qos, NULL) : -1;
    dds_delete_qos(str_qos);
    return int_participant;
}
