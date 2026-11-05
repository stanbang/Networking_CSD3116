/* Boundary checks fail with nonzero exit through assertions. Author: <team>, Codex assistance. Date: 2026-11-03. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "common/account_protocol.h"
#include "common/audit_log.h"
#include "common/protocol.h"

/* Missing and malformed handshakes must not allocate or adopt a slot. */
static void handshake_cases(void)
{
    Ast_Join str_join = {.int_version=AST_VERSION,.int_seq=1,.arr_nonce={1}};
    assert(join_valid(&str_join)); assert(!join_valid(NULL));
    str_join.arr_nonce[0]=0; assert(!join_valid(&str_join));
    str_join.arr_nonce[0]=1; str_join.int_seq=0; assert(!join_valid(&str_join));
    str_join.int_seq=1; str_join.int_version=1; assert(!join_valid(&str_join));
    Ast_Response str_response = {.int_version=AST_VERSION,.int_seq=1,.arr_nonce={1},
        .int_player_id=AST_PLAYERS,.int_session=1};
    assert(response_valid(&str_response)); assert(!response_valid(NULL));
    str_response.int_player_id=0; assert(!response_valid(&str_response));
    str_response.int_player_id=AST_PLAYERS+1; assert(!response_valid(&str_response));
    str_response.int_status=1; assert(!response_valid(&str_response));
    str_response.int_player_id=0; assert(!response_valid(&str_response));
    str_response.int_session=0; assert(response_valid(&str_response));
    str_response.int_status=2; assert(!response_valid(&str_response));
    str_response.int_status=1; str_response.int_seq=0; assert(!response_valid(&str_response));
}

/* Exercise trusted-index and flag boundaries before session lookup. */
static void input_cases(void)
{
    Ast_Input str_input = {.int_version=AST_VERSION, .int_player_id=1, .int_seq=1, .int_session=1};
    assert(input_valid(&str_input));
    str_input.int_player_id = 0; assert(!input_valid(&str_input));
    str_input.int_player_id = UINT16_MAX; assert(!input_valid(&str_input));
    str_input.int_player_id = AST_PLAYERS; assert(input_valid(&str_input));
    str_input.int_keys = 32; assert(!input_valid(&str_input));
    str_input.int_keys = AST_KEYS; assert(input_valid(&str_input));
    str_input.int_session = 0; assert(!input_valid(&str_input));
    str_input.int_session = 1; str_input.int_seq = 0; assert(!input_valid(&str_input));
    str_input.int_seq = 1; str_input.int_version = 1; assert(!input_valid(&str_input));
    assert(!input_valid(NULL)); assert(!leave_valid(NULL)); assert(!snapshot_valid(NULL));
    Ast_Leave str_leave = {.int_version=AST_VERSION, .int_player_id=5, .int_seq=1, .int_session=1};
    assert(leave_valid(&str_leave));
    str_leave.int_player_id = 6; assert(!leave_valid(&str_leave));
}

/* Sequence metadata must be checked before dereferencing the backing buffer. */
static void snapshot_cases(void)
{
    Ast_Ship arr_ships[2] = {{.int_player_id=1,.int_x=0,.int_y=0}, {.int_player_id=5,.int_x=79999,.int_y=59999}};
    Ast_Snapshot str_snapshot = {.int_version=AST_VERSION, .int_seq=1, .int_tick=1,.int_wave=1,
        .arr_ships={._maximum=2,._length=2,._buffer=arr_ships}};
    assert(snapshot_valid(&str_snapshot));
    str_snapshot.int_seq=0; assert(!snapshot_valid(&str_snapshot)); str_snapshot.int_seq=1;
    str_snapshot.int_tick=UINT64_MAX; assert(!snapshot_valid(&str_snapshot)); str_snapshot.int_tick=1;
    str_snapshot.arr_ships._maximum=1; assert(!snapshot_valid(&str_snapshot)); str_snapshot.arr_ships._maximum=2;
    arr_ships[1].int_player_id = 1; assert(!snapshot_valid(&str_snapshot));
    arr_ships[1].int_player_id = 5; arr_ships[1].int_x = 80000; assert(!snapshot_valid(&str_snapshot));
    arr_ships[1].int_x = 79999; arr_ships[1].int_y = -1; assert(!snapshot_valid(&str_snapshot));
    str_snapshot.arr_ships._length=6; assert(!snapshot_valid(&str_snapshot));
    str_snapshot.arr_ships._length=1; str_snapshot.arr_ships._buffer=NULL; assert(!snapshot_valid(&str_snapshot));
}

/* Malicious entity metadata must fail before any untrusted buffer is copied. */
static void entity_cases(void)
{
    Ast_Ship str_ship = {.int_player_id=1};
    Ast_Bullet arr_bullets[2] = {{.int_id=10,.int_owner=1},{.int_id=11,.int_owner=1}};
    Ast_Asteroid arr_asteroids[2] = {{.int_id=20,.int_size=1},{.int_id=21,.int_size=3}};
    Ast_Snapshot str_snapshot = {.int_version=AST_VERSION,.int_seq=1,.int_wave=1,
        .arr_ships={._maximum=1,._length=1,._buffer=&str_ship},
        .arr_bullets={._maximum=2,._length=2,._buffer=arr_bullets},
        .arr_asteroids={._maximum=2,._length=2,._buffer=arr_asteroids}};
    assert(snapshot_valid(&str_snapshot));
    str_snapshot.arr_bullets._length=AST_BULLETS+1; assert(!snapshot_valid(&str_snapshot));
    str_snapshot.arr_bullets._length=2; str_snapshot.arr_bullets._maximum=1; assert(!snapshot_valid(&str_snapshot));
    str_snapshot.arr_bullets._maximum=2; str_snapshot.arr_bullets._buffer=NULL; assert(!snapshot_valid(&str_snapshot));
    str_snapshot.arr_bullets._buffer=arr_bullets;
    arr_bullets[1].int_id=10; assert(!snapshot_valid(&str_snapshot));
    arr_bullets[1].int_id=0; assert(!snapshot_valid(&str_snapshot)); arr_bullets[1].int_id=11;
    arr_bullets[1].int_owner=0; assert(!snapshot_valid(&str_snapshot));
    arr_bullets[1].int_owner=AST_PLAYERS+1; assert(!snapshot_valid(&str_snapshot));
    arr_bullets[1].int_owner=2; assert(!snapshot_valid(&str_snapshot)); arr_bullets[1].int_owner=1;
    arr_bullets[1].int_x=AST_WIDTH*100; assert(!snapshot_valid(&str_snapshot)); arr_bullets[1].int_x=0;
    str_snapshot.arr_asteroids._length=AST_ASTEROIDS+1; assert(!snapshot_valid(&str_snapshot));
    str_snapshot.arr_asteroids._length=2; str_snapshot.arr_asteroids._maximum=1; assert(!snapshot_valid(&str_snapshot));
    str_snapshot.arr_asteroids._maximum=2; str_snapshot.arr_asteroids._buffer=NULL; assert(!snapshot_valid(&str_snapshot));
    str_snapshot.arr_asteroids._buffer=arr_asteroids;
    arr_asteroids[1].int_id=20; assert(!snapshot_valid(&str_snapshot));
    arr_asteroids[1].int_id=0; assert(!snapshot_valid(&str_snapshot)); arr_asteroids[1].int_id=21;
    arr_asteroids[1].int_size=0; assert(!snapshot_valid(&str_snapshot));
    arr_asteroids[1].int_size=4; assert(!snapshot_valid(&str_snapshot)); arr_asteroids[1].int_size=3;
    arr_asteroids[1].int_y=-1; assert(!snapshot_valid(&str_snapshot)); arr_asteroids[1].int_y=0;
    str_ship.int_lives=AST_LIVES+1; assert(!snapshot_valid(&str_snapshot)); str_ship.int_lives=AST_LIVES;
    str_ship.int_respawn=AST_RESPAWN_TICKS+1; assert(!snapshot_valid(&str_snapshot)); str_ship.int_respawn=0;
    str_ship.int_invulnerable=AST_INVULNERABLE_TICKS+1; assert(!snapshot_valid(&str_snapshot)); str_ship.int_invulnerable=0;
    str_ship.int_vx=INT16_MAX; assert(!snapshot_valid(&str_snapshot)); str_ship.int_vx=0;
    assert(snapshot_valid(&str_snapshot));
    str_snapshot.int_reconnect_seconds=30; assert(snapshot_valid(&str_snapshot));
    str_snapshot.int_reconnect_seconds=31; assert(!snapshot_valid(&str_snapshot));
    str_snapshot.int_reconnect_seconds=0;
    puts("PASS reconnect countdown bounded to30seconds");
    puts("PASS malformed bullet/asteroid/health snapshot boundaries");
}

/* Existing evidence and symlink targets survive rejected log creation. */
static void audit_creation_cases(void)
{
    char str_dir[] = "/tmp/asteroids-audit-test.XXXXXX";
    assert(mkdtemp(str_dir));
    char str_path[256], str_link[256], str_text[32] = {0};
    assert(snprintf(str_path, sizeof(str_path), "%s/evidence", str_dir) > 0);
    assert(snprintf(str_link, sizeof(str_link), "%s/link", str_dir) > 0);
    Audit str_first = {0}, str_rejected = {0};
    assert(audit_open(&str_first, str_path));
    assert(!audit_open(&str_rejected, str_path));
    assert(audit_close(&str_first));
    FILE *str_file = fopen(str_path, "w"); assert(str_file);
    assert(fputs("KEEP EVIDENCE\n", str_file) >= 0); assert(fclose(str_file) == 0);
    assert(symlink(str_path, str_link) == 0);
    assert(!audit_open(&str_rejected, str_link));
    str_file = fopen(str_path, "r"); assert(str_file);
    assert(fgets(str_text, sizeof(str_text), str_file)); assert(fclose(str_file) == 0);
    assert(strcmp(str_text, "KEEP EVIDENCE\n") == 0);
    assert(unlink(str_link) == 0); assert(unlink(str_path) == 0); assert(rmdir(str_dir) == 0);
    puts("PASS audit exclusive creation and symlink target preservation");
}

static void account_cases(void)
{
    assert(!account_password_valid((const uint8_t *)"abcde", 5));
    assert(account_password_valid((const uint8_t *)"abcdef", 6));
    assert(account_password_valid((const uint8_t *)"123456", 6));
    puts("PASS six-digit password accepted");
    assert(account_password_valid((const uint8_t *)"abcdefg", 7));
    puts("PASS password minimum: five rejected, six/seven letters accepted");
    AstAccount_Request str_req = {.int_version = AstAccount_VERSION, .int_operation = AstAccount_LOGIN,
        .int_seq = 1, .arr_client_nonce = {1}, .arr_request_nonce = {2}, .arr_server_epoch = {3},
        .arr_server_challenge = {4}, .int_username_length = 24, .int_password_length = 128};
    memset(str_req.arr_username, 'a', sizeof(str_req.arr_username));
    memset(str_req.arr_password, '~', sizeof(str_req.arr_password));
    assert(account_request_valid(&str_req));
    str_req.int_password_length = 129; assert(!account_request_valid(&str_req));
    str_req.int_password_length = 0; assert(!account_request_valid(&str_req));
    str_req.int_password_length = 128;
    str_req.int_username_length = 25; assert(!account_request_valid(&str_req));
    str_req.int_username_length = 0; assert(!account_request_valid(&str_req));
    str_req.int_username_length = 24;
    str_req.arr_username[0] = 0xc3; assert(!account_request_valid(&str_req));
    str_req.arr_username[0] = 'A'; assert(!account_request_valid(&str_req));
    str_req.arr_username[0] = 'a';
    str_req.arr_password[0] = 0; assert(!account_request_valid(&str_req));
    str_req.arr_password[0] = ' ';
    str_req.int_operation = UINT16_MAX; assert(!account_request_valid(&str_req));
    str_req.int_operation = AstAccount_LOGIN;
    str_req.int_seq = 0; assert(!account_request_valid(&str_req));
    str_req.int_seq = 1;
    str_req.arr_account_session[0] = 1; assert(!account_request_valid(&str_req));
    str_req.arr_account_session[0] = 0;
    memset(str_req.arr_username + 3, 0, 21);
    memset(str_req.arr_password + 12, 0, 116);
    str_req.int_username_length = 3; str_req.int_password_length = 12;
    assert(account_request_valid(&str_req));
    str_req.arr_password[127] = 'x'; assert(!account_request_valid(&str_req));

    AstAccount_Response str_rsp = {.int_version = AstAccount_VERSION, .int_operation = AstAccount_LEADERBOARD,
        .int_seq = 1, .int_request_seq = 2, .arr_client_nonce = {1}, .arr_request_nonce = {2},
        .arr_server_epoch = {3}, .arr_server_challenge = {4}, .b_persisted = true,
        .int_account_id = 1, .int_username_length = 3, .arr_username = {'a', 'b', 'c'}};
    assert(account_response_valid(&str_rsp));
    str_rsp.int_entry_count = 11; assert(!account_response_valid(&str_rsp));
    str_rsp.int_entry_count = 0;
    str_rsp.int_account_id = UINT64_MAX; assert(!account_response_valid(&str_rsp));
    str_rsp.int_account_id = 1;
    str_rsp.int_status = UINT16_MAX; assert(!account_response_valid(&str_rsp));
    puts("PASS account length, ASCII, role, sequence and leaderboard boundaries");
}

/* Assertions intentionally abort with nonzero status on validation regressions. */
int main(void)
{
    handshake_cases(); input_cases(); snapshot_cases(); entity_cases(); audit_creation_cases(); account_cases();
    /* Regression: worker receives heartbeat after main sampled its tick time. */
    assert(!lease_expired(100, 101));
    assert(!lease_expired(AST_LEASE_NS - 1, 0));
    assert(lease_expired(AST_LEASE_NS, 0));
    char *arr_bad[] = {"client", "--identity", "0"}; Options str_options = {0};
    assert(!options_parse(3, arr_bad, &str_options, false));
    char *arr_good[] = {"client", "--identity", "6", "--headless", "--duration", "2"};
    assert(options_parse(6, arr_good, &str_options, false));
    assert(!str_options.b_test_gameover);
    char *arr_scenario[] = {"server", "--test-scenario", "gameover"};
    assert(options_parse(3, arr_scenario, &str_options, true) && str_options.b_test_gameover);
    assert(!options_parse(3, arr_scenario, &str_options, false));
    puts("PASS protocol boundaries and CLI validation");
    return 0;
}
