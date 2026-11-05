/* Finite client entry point, headless or GLFW. Author: <team>, Codex assistance. Date: 2026-11-03. */
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <openssl/crypto.h>
#include "client/client.h"
static volatile sig_atomic_t b_signal = 0;

/* Signal-safe request is observed only by main, then handed to worker atomically. */
static void stop_signal(int int_signal) { (void)int_signal; b_signal = 1; }

/* Main-thread rendering can observe the signal-safe stop flag. */
bool client_interrupted(void) { return b_signal != 0; }

/* Bot input is state, never movement; simulation consumes it once per tick in P3. */
static void headless_run(Client *str_client, uint64_t int_deadline)
{
    while (!b_signal && !atomic_load(&str_client->b_stop) && monotonic_ns() < int_deadline) {
        atomic_store(&str_client->int_keys, client_bot_keys(str_client));
        sleep_ns(UINT64_C(1000000000) / AST_HZ);
    }
}

/* Start network once; join ensures argv and shared state outlive all worker accesses. */
int main(int int_argc, char **arr_argv)
{
    Client str_client = {.str_mutex=PTHREAD_MUTEX_INITIALIZER};
    pthread_t str_thread;
    setvbuf(stdout, NULL, _IOLBF, 0);
    if (!options_parse(int_argc, arr_argv, &str_client.str_options, false)) { fprintf(stderr, "Invalid client options\n"); return 2; }
    if (signal(SIGINT, stop_signal) == SIG_ERR || signal(SIGTERM, stop_signal) == SIG_ERR) return 1;
    if (!event_queue_init(&str_client.str_events)) return 1;
    if (str_client.str_options.int_credentials_fd >= 0 && !login_read_credentials(&str_client, str_client.str_options.int_credentials_fd)) {
        fputs("Invalid private credential input\n", stderr);
        OPENSSL_cleanse(&str_client.str_login_command, sizeof(str_client.str_login_command));
        (void)event_queue_fini(&str_client.str_events);
        return 2;
    }
    int int_result = pthread_create(&str_thread, NULL, client_worker, &str_client);
    if (int_result) {
        fprintf(stderr, "pthread_create: %s\n", strerror(int_result));
        OPENSSL_cleanse(&str_client.str_login_command, sizeof(str_client.str_login_command));
        (void)event_queue_fini(&str_client.str_events);
        return 1;
    }
    uint64_t int_deadline = monotonic_ns() + (uint64_t)(str_client.str_options.dbl_duration * 1e9);
    bool b_ok = true;
    if (str_client.str_options.b_headless) headless_run(&str_client, int_deadline);
    else b_ok = render_run(&str_client, int_deadline);
    atomic_store(&str_client.b_stop, true);
    if (!event_queue_mark(&str_client.str_events, CLIENT_LOGIN_EVENT)) b_ok = false;
    int_result = pthread_join(str_thread, NULL);
    if (int_result) { fprintf(stderr, "pthread_join: %s\n", strerror(int_result)); return 1; }
    puts("CLIENT_EXIT");
    OPENSSL_cleanse(&str_client.str_login_command, sizeof(str_client.str_login_command));
    if (!event_queue_fini(&str_client.str_events)) b_ok = false;
    if (pthread_mutex_destroy(&str_client.str_mutex) != 0) b_ok = false;
    if (str_client.str_options.b_headless && !atomic_load(&str_client.b_joined_once) &&
        !atomic_load(&str_client.b_rejected) && !atomic_load(&str_client.b_account_complete)) {
        fprintf(stderr, "CLIENT_AUTH_OR_SERVER_TIMEOUT no_session\n"); return 1;
    }
    return !b_ok || atomic_load(&str_client.b_failed) ? 1 : 0;
}
