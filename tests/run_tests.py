#!/usr/bin/env python3
"""Portable, dependency-free regression tests for the original 9x9 C game."""
import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess
import sys
import tempfile
import time
import unittest

HERE = Path(__file__).resolve().parent
SOURCE = HERE.parent / "connect4.c"
TIMEOUT = float(os.environ.get("TEST_TIMEOUT", "5"))
OUTPUT_LIMIT = 1024 * 1024
DRAW = [
    2, 4, 5, 2, 1, 8, 6, 7, 7, 7, 4, 3, 9, 9, 2, 2, 9, 9,
    9, 4, 3, 5, 2, 9, 5, 3, 7, 5, 8, 6, 9, 7, 4, 1, 1, 4,
    5, 9, 9, 2, 3, 5, 2, 1, 1, 3, 5, 2, 7, 5, 2, 1, 1, 8,
    6, 1, 8, 7, 1, 6, 6, 8, 3, 3, 8, 8, 3, 3, 7, 7, 4, 6,
    8, 4, 5, 8, 4, 4, 6, 6, 6,
]
LAST_CELL_WIN = DRAW[:68] + [4, 8, 8, 6, 7, 6, 6, 6, 4, 7, 4, 5, 4]
WIN_CASES = {
    "horizontal_endpoint": [3, 8, 4, 8, 5, 7, 6],
    "horizontal_join": [3, 8, 4, 8, 6, 7, 5],
    "vertical": [3, 8, 3, 8, 3, 7, 3],
    "diagonal_rising_right": [3, 4, 4, 5, 7, 5, 5, 6, 6, 6, 6],
    "diagonal_rising_left": [7, 6, 6, 5, 3, 5, 5, 4, 4, 4, 4],
    "diagonal_join_right": [3, 4, 4, 5, 7, 6, 6, 6, 6, 5, 5],
    "diagonal_join_left": [7, 6, 6, 5, 3, 4, 4, 4, 4, 5, 5],
    "left_edge": [1, 9, 2, 9, 3, 8, 4],
    "right_edge": [9, 1, 8, 1, 7, 2, 6],
    "left_column": [1, 9, 1, 9, 1, 8, 1],
    "right_column": [9, 1, 9, 1, 9, 2, 9],
}


def encode(moves):
    return "".join("{}\n".format(move) for move in moves)


def model(moves):
    """Check fixture legality, no early wins, and every gravity placement."""
    board = [["-"] * 9 for _ in range(9)]
    winner = None
    for turn, col in enumerate(moves):
        assert winner is None, "fixture continues beyond a win"
        assert 1 <= col <= 9, "fixture has an invalid column"
        x = col - 1
        empty = [y for y in range(9) if board[y][x] == "-"]
        assert empty, "fixture overfills a column"
        y = max(empty)
        chip = "Y" if turn % 2 == 0 else "R"
        board[y][x] = chip
        # Enumerate complete four-cell segments; independent of game internals.
        for yy in range(9):
            for xx in range(9):
                for dx, dy in ((1, 0), (0, 1), (1, 1), (1, -1)):
                    cells = [(xx + i * dx, yy + i * dy) for i in range(4)]
                    if all(0 <= a < 9 and 0 <= b < 9 for a, b in cells):
                        if all(board[b][a] == chip for a, b in cells):
                            winner = chip
    return "\n".join("".join(row) for row in board), winner


def run_bounded(executable, data=""):
    """Bound both time and output, including against the old input busy-loop."""
    with tempfile.TemporaryFile() as stdout, tempfile.TemporaryFile() as stderr:
        process = subprocess.Popen([str(executable)], stdin=subprocess.PIPE,
                                   stdout=stdout, stderr=stderr)
        problem = None
        start = time.monotonic()
        try:
            try:
                process.stdin.write(data.encode())
                process.stdin.close()
            except BrokenPipeError:
                pass
            while process.poll() is None:
                if time.monotonic() - start > TIMEOUT:
                    problem = "timed out after {} seconds".format(TIMEOUT)
                    break
                if os.fstat(stdout.fileno()).st_size + os.fstat(stderr.fileno()).st_size > OUTPUT_LIMIT:
                    problem = "exceeded {} bytes of output (possible busy-loop)".format(OUTPUT_LIMIT)
                    break
                time.sleep(0.01)
        finally:
            if process.poll() is None:
                process.kill()
            process.wait()
            if process.stdin and not process.stdin.closed:
                process.stdin.close()
        stdout.seek(0)
        stderr.seek(0)
        out = stdout.read(OUTPUT_LIMIT).decode(errors="replace")
        err = stderr.read(OUTPUT_LIMIT).decode(errors="replace")
        if problem:
            raise AssertionError("{}: {}\n{}".format(executable.name, problem, err[-2000:]))
        if process.returncode != 0:
            raise AssertionError("{} exited {}\n{}\n{}".format(
                executable.name, process.returncode, err[-5000:], out[-2000:]))
        if err:
            raise AssertionError("unexpected stderr from {}:\n{}".format(executable.name, err[-5000:]))
        return out


class RegressionTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.build = tempfile.TemporaryDirectory(prefix="connect4-regression-")
        cls.addClassCleanup(cls.build.cleanup)
        directory = Path(cls.build.name)
        cls.game = directory / "connect4"
        cls.harness = directory / "win_harness"
        cc = shlex.split(os.environ.get("CC", "cc"))
        cppflags = shlex.split(os.environ.get("CPPFLAGS", ""))
        cflags = shlex.split(os.environ.get("CFLAGS", "-O2"))
        ldflags = shlex.split(os.environ.get("LDFLAGS", ""))
        common = cc + ["-std=c11", "-Wall", "-Wextra", "-Wpedantic"] + cppflags + cflags
        escaped = str(SOURCE).replace("\\", "\\\\").replace('"', '\\"')
        for source, target, defines in (
            (SOURCE, cls.game, []),
            (HERE / "win_harness.c", cls.harness, ['-DCONNECT4_SOURCE="{}"'.format(escaped)]),
        ):
            command = common + defines + [str(source), "-o", str(target)] + ldflags
            result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                    text=True, timeout=60)
            if result.returncode:
                raise RuntimeError("compile failed:\n{}\n{}".format(" ".join(command), result.stderr))
            if result.stderr:
                sys.stderr.write(result.stderr)

    def check_game(self, data, accepted_moves, terminal=None):
        expected_board, expected_winner = model(accepted_moves)
        self.assertEqual(expected_winner, terminal if terminal in ("Y", "R") else None,
                         "independent fixture winner")
        out = run_bounded(self.game, data)
        boards = re.findall(r"(?m)^(?:[YR-]{9}\n){8}[YR-]{9}(?=\n|$)", out)
        self.assertTrue(boards, "game should display its 9 by 9 board")
        self.assertEqual(boards[-1], expected_board, "final board differs from legal accepted moves")
        winners = re.findall(r"\b([YR]) wins\b", out)
        self.assertEqual(winners, [terminal] if terminal in ("Y", "R") else [])
        is_draw = bool(re.search(r"\bdraw\b|\btie\b", out, flags=re.IGNORECASE))
        self.assertEqual(is_draw, terminal == "draw")
        return out

    def test_01_external_board_harness(self):
        out = run_bounded(self.harness)
        self.assertRegex(out, r"PASS: \d+ board and win checks")
        print(out.strip(), flush=True)

    def test_02_ordinary_joined_and_edge_wins(self):
        for name, moves in WIN_CASES.items():
            with self.subTest(name=name):
                self.check_game(encode(moves), moves, "Y")

    def test_03_no_false_early_wins(self):
        for name, moves in WIN_CASES.items():
            with self.subTest(name=name):
                self.check_game(encode(moves[:-1]), moves[:-1])

    def test_04_full_column_retries_same_player(self):
        # Nine alternating chips fill column 5. The tenth input must not count
        # as a move; R then wins vertically in column 3.
        accepted = [5] * 9 + [3, 8, 3, 8, 3, 7, 3]
        out = self.check_game(encode([5] * 10 + accepted[9:]), accepted, "R")
        self.assertRegex(out.lower(), r"col(?:umn)?.*full|full.*col(?:umn)?")

    def test_05_invalid_numbers_retry_same_player(self):
        moves = WIN_CASES["horizontal_endpoint"]
        self.check_game("0\n-1\n10\n2147483647\n-2147483648\n" + encode(moves), moves, "Y")

    def test_06_letters_retry_same_player(self):
        moves = WIN_CASES["horizontal_endpoint"]
        self.check_game("x\nletters\n" + encode(moves), moves, "Y")

    def test_07_eof_terminates_cleanly(self):
        for label, data, accepted in (
            ("empty", "", []),
            ("after one move", "3\n", [3]),
            ("no final newline", "3", [3]),
            ("after letters", "invalid", []),
            ("after invalid numbers", "0\n-1\n10", []),
            ("after whitespace", " \t\n\n", []),
            ("after full column", encode([5] * 10), [5] * 9),
        ):
            with self.subTest(name=label):
                self.check_game(data, accepted)

    def test_08_draw_fixture_is_a_legal_full_board(self):
        self.assertEqual(len(DRAW), 81)
        board, winner = model(DRAW)
        self.assertIsNone(winner)
        self.assertNotIn("-", board)
        self.assertEqual(board.count("Y"), 41)
        self.assertEqual(board.count("R"), 40)

    def test_09_no_draw_with_one_empty_cell(self):
        self.check_game(encode(DRAW[:-1]), DRAW[:-1])

    def test_10_full_board_draw_terminates(self):
        out = self.check_game(encode(DRAW), DRAW, "draw")
        self.assertEqual(len(re.findall(r"[YR] turn", out)), 81,
                         "a full board must end without asking for another move")

    def test_11_draw_ignores_postgame_input(self):
        out = self.check_game(encode(DRAW + [5, 1]), DRAW, "draw")
        self.assertEqual(len(re.findall(r"[YR] turn", out)), 81)

    def test_12_huge_and_malformed_tokens(self):
        moves = WIN_CASES["horizontal_endpoint"]
        invalid = ["9" * 512, "-" + "9" * 512, "3x", "1.5", "+", "x" * 512]
        self.check_game("\n".join(invalid) + "\n" + encode(moves), moves, "Y")

    def test_13_whitespace_separated_moves(self):
        moves = WIN_CASES["horizontal_endpoint"]
        self.check_game(" \t\n" + " \t".join(map(str, moves)), moves, "Y")

    def test_14_last_cell_win_takes_priority_over_draw(self):
        self.assertEqual(len(LAST_CELL_WIN), 81)
        board, winner = model(LAST_CELL_WIN)
        self.assertNotIn("-", board)
        self.assertEqual(winner, "Y")
        out = self.check_game(encode(LAST_CELL_WIN), LAST_CELL_WIN, "Y")
        self.assertEqual(len(re.findall(r"[YR] turn", out)), 81)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=SOURCE,
                        help="alternate connect4.c file (default: ../connect4.c)")
    args, unittest_args = parser.parse_known_args()
    SOURCE = args.source.resolve()
    unittest.main(argv=[sys.argv[0]] + unittest_args, verbosity=2)
