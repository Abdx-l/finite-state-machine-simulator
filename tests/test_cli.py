"""End-to-end CLI checks with only the Python standard library."""

import os
from pathlib import Path
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]


class SimulatorTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory()
        cls.binary = Path(cls.temp.name) / ("fsm.exe" if os.name == "nt" else "fsm")
        subprocess.run(
            [os.environ.get("CC", "gcc"), "-std=c11", "-Wall", "-Wextra",
             "-Werror", "-pedantic", str(ROOT / "src" / "fsm.c"),
             "-o", str(cls.binary)],
            check=True,
        )

    @classmethod
    def tearDownClass(cls):
        cls.temp.cleanup()

    def run_machine(self, commands, *args):
        return subprocess.run(
            [str(self.binary), *args],
            input=commands, text=True, capture_output=True, check=True,
        ).stdout

    def test_rewire_run_and_prune(self):
        output = self.run_machine(
            "set 0 0 1\nset 1 1 2\nrun 01\nreachable\nprune\nprint\n"
            "set 0 1 5\nreset\nstep 0\nquit\n", "6"
        )
        self.assertIn("current=2", output)
        self.assertIn("unreachable: 3 4 5", output)
        self.assertIn("pruned 3 state(s)", output)
        self.assertNotIn("\n5: 0->", output)
        self.assertIn("error: invalid command or inactive state", output)
        self.assertTrue(output.rstrip().endswith("current=1"))

    def test_bad_input_does_not_change_current_state(self):
        output = self.run_machine("run 010x\nstep 2\nset 0 1 999\nprint\n")
        self.assertIn("error: run expects only 0 and 1", output)
        self.assertIn("start=0 current=0", output)
        self.assertIn("0: 0->0 1->0", output)

    def test_start_changes_reachability(self):
        output = self.run_machine("start 2\nreachable\n")
        self.assertIn("start=2 current=2", output)
        self.assertIn("unreachable: 0 1 3 4 5", output)


if __name__ == "__main__":
    unittest.main()

