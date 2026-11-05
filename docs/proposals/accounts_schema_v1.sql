-- J0 REVIEW DRAFT ONLY. No production database is created. Team + AI, 2026-11-05.
PRAGMA foreign_keys = ON;
PRAGMA journal_mode = DELETE;
PRAGMA synchronous = FULL;
BEGIN IMMEDIATE;
CREATE TABLE accounts (
    account_id INTEGER PRIMARY KEY,
    username TEXT NOT NULL UNIQUE CHECK (
        length(username) BETWEEN 3 AND 24 AND
        username NOT GLOB '*[^a-z0-9_]*' AND instr(username, char(0)) = 0
    ),
    identity_channel INTEGER NOT NULL UNIQUE CHECK (identity_channel BETWEEN 1 AND 6),
    password_salt BLOB NOT NULL CHECK (length(password_salt) = 16),
    password_verifier BLOB NOT NULL CHECK (length(password_verifier) = 32),
    scrypt_n INTEGER NOT NULL CHECK (scrypt_n = 131072),
    scrypt_r INTEGER NOT NULL CHECK (scrypt_r = 8),
    scrypt_p INTEGER NOT NULL CHECK (scrypt_p = 1),
    password_generation INTEGER NOT NULL DEFAULT 1 CHECK (password_generation > 0),
    created_unix_s INTEGER NOT NULL CHECK (created_unix_s >= 0),
    UNIQUE (account_id, identity_channel)
);
CREATE TRIGGER accounts_capacity BEFORE INSERT ON accounts
WHEN (SELECT count(*) FROM accounts) >= 6
BEGIN
    SELECT RAISE(ABORT, 'account capacity reached');
END;
CREATE TABLE best_scores (
    account_id INTEGER PRIMARY KEY REFERENCES accounts(account_id),
    best_score INTEGER NOT NULL CHECK (best_score BETWEEN 0 AND 4294967295),
    source_round_key BLOB NOT NULL CHECK (length(source_round_key) = 24),
    source_game_session BLOB NOT NULL CHECK (length(source_game_session) = 8),
    updated_unix_s INTEGER NOT NULL CHECK (updated_unix_s >= 0)
);
CREATE TABLE resume_tokens (
    account_id INTEGER PRIMARY KEY,
    token_hash BLOB NOT NULL UNIQUE CHECK (length(token_hash) = 32),
    identity_channel INTEGER NOT NULL CHECK (identity_channel BETWEEN 1 AND 6),
    password_generation INTEGER NOT NULL CHECK (password_generation > 0),
    issued_unix_s INTEGER NOT NULL CHECK (issued_unix_s >= 0),
    expires_unix_s INTEGER NOT NULL CHECK (expires_unix_s > issued_unix_s),
    FOREIGN KEY (account_id, identity_channel) REFERENCES accounts(account_id, identity_channel)
);
PRAGMA user_version = 1;
COMMIT;
