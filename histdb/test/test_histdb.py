import os
import sqlite3
import subprocess

from datetime import datetime
from datetime import timedelta
from getpass import getuser
from os import path
from pathlib import Path
from typing import Iterable
from typing import Optional
from typing import Tuple
from typing import Union

import pyrfc3339
import pytest


DIR_OF_THIS_SCRIPT = path.abspath(path.dirname(__file__))

# TESTDB = path.join(DIR_OF_THIS_SCRIPT, "test.sqlite3")

HISTDB_BINARY = path.abspath(
    path.join(
        DIR_OF_THIS_SCRIPT,
        "../build/stage/bin/histdb",
    )
)


def get_conn() -> sqlite3.Connection:
    conn = sqlite3.connect("test.sqlite3")
    conn.row_factory = sqlite3.Row
    return conn


# TODO:
#   1. explicitly set the database path
#   2. assert error message contents
def histdb(args: Iterable) -> str:
    env = os.environ.copy()
    env["HISTDB_PROD"] = "0"
    if type(args) is str:
        args = [args]
    return subprocess.check_output(
        [HISTDB_BINARY] + [str(a) for a in args],
        encoding="utf-8",
        stderr=subprocess.STDOUT,
        env=env,
    )


def new_session_id() -> int:
    sid = int(histdb("session").rstrip())
    assert sid >= 1
    return sid


# def histdb_insert(
#     session_id: Union[str, int], status_code: Union[str, int], raw: str
# ) -> None:
#     args = [
#         "insert",
#         "--session",
#         str(session_id),
#         "--status-code",
#         str(status_code),
#         raw,
#     ]
#     assert histdb(args) == ""


def test_histdb_binary():
    assert histdb("info") != ""


def test_histdb_help():
    assert histdb(["--help"]) != ""
    assert histdb(["-h"]) != ""
    for cmd in ["session", "info", "insert", "boot-id"]:
        assert histdb([cmd, "--help"]) != ""
        assert histdb([cmd, "-h"]) != ""


def test_histdb_session_id(monkeypatch, tmpdir: Path) -> None:
    monkeypatch.chdir(tmpdir)

    assert new_session_id() == 1
    assert new_session_id() == 2
    assert new_session_id() == 3

    cur = get_conn().cursor()
    cur.execute(
        "SELECT * FROM session_ids WHERE id IN (1, 2, 3)",
    )
    rows = cur.fetchall()
    assert len(rows) == 3
    for i, row in enumerate(rows):
        assert row["id"] == i + 1
        assert row["ppid"] == os.getpid()
        assert row["boot_time"]  # TODO: actually assert on this


def test_histdb_session_id_eval(monkeypatch, tmpdir: Path) -> None:
    monkeypatch.chdir(tmpdir)
    assert histdb(["session", "--eval"]).rstrip() == "export HISTDB_SESSION_ID=1;"


def assert_not_exists(
    query: str, args: Optional[Tuple[Union[str, int], ...]] = None
) -> None:
    cur = get_conn().cursor()
    cur.execute(query, args if args else ())
    assert cur.fetchone() is None


def test_histdb_invalid_session_id(monkeypatch, tmpdir: Path) -> None:
    monkeypatch.chdir(tmpdir)

    # create a new session_id to initialize the tables
    new_session_id()
    for session_id in [-1, 99999999]:
        args = [
            "insert",
            f"--session={session_id}",
            "--status-code=1234",
            "1 SHOULD NOT EXIST",
        ]
        with pytest.raises(subprocess.SubprocessError):
            assert histdb(args)

        assert_not_exists(
            "SELECT * FROM history WHERE raw like (?) OR session_id = (?)",
            ("%SHOULD NOT EXIST%", session_id),
        )


def test_histdb_invalid_history_id(monkeypatch, tmpdir: Path) -> None:
    monkeypatch.chdir(tmpdir)

    session_id = new_session_id()
    for hist_id in ["", "a", "123a", "0x1", "-123"]:
        args = [
            "insert",
            f"--session={session_id}",
            "--status-code=1234",
            f"{hist_id} INVALID HIST ID",
        ]
        with pytest.raises(subprocess.SubprocessError):
            histdb(args)

        assert_not_exists(
            "SELECT * FROM history WHERE raw like (?) OR session_id = (?)",
            ("%INVALID HIST ID%", session_id),
        )


def test_histdb_empty_raw_history(monkeypatch, tmpdir: Path) -> None:
    monkeypatch.chdir(tmpdir)
    session_id = new_session_id()
    for raw in ["", " ", "\t", "\n"]:
        args = [
            "insert",
            f"--session={session_id}",
            "--status-code=1234",
            f"123{raw}",
        ]
        with pytest.raises(subprocess.SubprocessError):
            histdb(args)

        assert_not_exists(
            "SELECT * FROM history WHERE session_id = (?)",
            (session_id,),
        )


def assert_rfc3339_within(ts: str, seconds: int = 1) -> None:
    created_at: datetime = pyrfc3339.parse(ts)
    delta = datetime.now(created_at.tzinfo) - created_at
    assert delta > timedelta()
    assert delta < timedelta(seconds=seconds)


def test_histdb_insert(monkeypatch, tmpdir: Path) -> None:
    monkeypatch.chdir(tmpdir)
    session_id = new_session_id()
    args = [
        "insert",
        "--session",
        session_id,
        "--status-code",
        0,
        "1 cd /usr/local/bin",
    ]
    assert histdb(args) == ""

    cur = get_conn().cursor()
    cur.execute(
        "SELECT * FROM history ORDER BY id DESC LIMIT 1",
    )
    row = cur.fetchone()

    assert row["id"] == 1
    assert row["session_id"] == int(session_id)
    assert row["history_id"] == 1
    assert row["ppid"] == os.getpid()
    assert row["status_code"] == 0
    assert_rfc3339_within(row["created_at"], seconds=5)
    assert row["username"] == getuser()
    assert row["directory"] == DIR_OF_THIS_SCRIPT
    assert row["raw"] == "cd /usr/local/bin"

    # Make sure created_at is close to now
    created_at: datetime = pyrfc3339.parse(row["created_at"])
    delta = datetime.now(created_at.tzinfo) - created_at
    assert delta > timedelta()
    assert delta < timedelta(seconds=5)


def test_histdb_insert_many(monkeypatch, tmpdir: Path) -> None:
    monkeypatch.chdir(tmpdir)
    session_id = new_session_id()

    for x in range(1, 10):
        args = [
            "insert",
            "--session",
            session_id,
            "--status-code",
            x,
            f"{x} echo {x}",
        ]
        assert histdb(args) == ""

    cur = get_conn().cursor()
    cur.execute(
        "SELECT * FROM history WHERE session_id = (?) ORDER BY id",
        (session_id,),
    )
    rows = cur.fetchall()
    assert len(rows) == 9

    for i, row in enumerate(rows):
        x = i + 1
        assert row["id"] == x
        assert row["session_id"] == int(session_id)
        assert row["history_id"] == x
        assert row["ppid"] == os.getpid()
        assert row["status_code"] == x
        assert_rfc3339_within(row["created_at"], seconds=10)
        assert row["username"] == getuser()
        assert row["directory"] == DIR_OF_THIS_SCRIPT
        assert row["raw"] == f"echo {x}"


def test_histdb_boot_id(monkeypatch, tmpdir: Path) -> None:
    monkeypatch.chdir(tmpdir)

    assert int(histdb("boot-id").rstrip()) == 1
    assert int(histdb("boot-id").rstrip()) == 2
    assert int(histdb("boot-id").rstrip()) == 3

    cur = get_conn().cursor()
    cur.execute("SELECT * FROM boot_ids;")
    rows = cur.fetchall()
    assert len(rows) == 3

    for row in rows:
        assert row["id"] in (1, 2, 3)
        assert_rfc3339_within(row["created_at"], seconds=10)


def test_histdb_boot_id_eval(monkeypatch, tmpdir: Path) -> None:
    monkeypatch.chdir(tmpdir)
    assert histdb(["boot-id", "--eval"]) == "export HISTDB_BOOT_ID=1;\n"


def test_histdb_schema_migrations() -> None:
    pytest.skip("TODO")


def test_histdb_info() -> None:
    pytest.skip("TODO")
